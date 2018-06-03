#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/readonly_memoryview.hpp>

#include <boost/circular_buffer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <list>
#include <memory>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

using namespace std;
using namespace boost::uuids;
namespace py = pybind11;


class allocated_block
{
public:
    allocated_block(size_t capacity)
        :
            shm_key_(generate_shm_key()),
            capacity_(capacity)
    {
        auto fd = ::shm_open(shm_key_.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        if (fd == FAILURE)
        {
            std::stringstream ss;
            ss << "shm_open: error(" << errno << ") " << ::strerror(errno);
            throw std::invalid_argument(ss.str());
        }

        auto result = ::ftruncate(fd, capacity_);
        if (result == FAILURE)
        {
            std::stringstream ss;
            ss << "ftruncate: error(" << errno << ") " << ::strerror(errno);
            throw std::invalid_argument(ss.str());
        }

        // first mapping uses 2 * capacity to establish space also for the second mapping
        // as use of MAP_FIXED requires use of address which is valid. Otherwise on rare occasions
        // second mapping could be established in boundries which aren't valid
        auto map = ::mmap(NULL, 2ul * capacity_, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (map == MAP_FAILED)
        {
            std::stringstream ss;
            ss << "mmap: error(" << errno << ") " << ::strerror(errno);
            throw std::invalid_argument(ss.str());
        }

        first_map_ = reinterpret_cast<uint8_t*>(map);

        auto expected_second_map = first_map_ + capacity_;
        map = ::mmap(expected_second_map, capacity_, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0);
        if (map == MAP_FAILED)
        {
            std::stringstream ss;
            ss << "mmap: error(" << errno << ") " << ::strerror(errno);
            throw std::invalid_argument(ss.str());
        }
        else if (map != expected_second_map)
        {
            std::stringstream ss;
            ss << "mmap: requested mapped block 0x" << std::hex << expected_second_map << " but is 0x" << std::hex << map;
            throw std::invalid_argument(ss.str());
        }

        second_map_ = reinterpret_cast<uint8_t*>(map);
    }

    ~allocated_block()
    {
        if (second_map_ != nullptr)
        {
            ::munmap(second_map_, capacity_);
            second_map_ = nullptr;
        }

        if (first_map_ != nullptr)
        {
            ::munmap(first_map_, capacity_);
            first_map_ = nullptr;
        }

        ::shm_unlink(shm_key_.c_str());
    }

    template <typename pointer_type>
    pointer_type pointer()
    {
        return reinterpret_cast<pointer_type>(first_map_);
    }

private:
    static constexpr auto FAILURE = -1;

    static string generate_shm_key()
    {
        static random_generator generator;
        auto id = generator();

        return boost::lexical_cast<string>(id);
    }

    const string shm_key_;
    const size_t capacity_;
    uint8_t* first_map_ {nullptr};
    uint8_t* second_map_ {nullptr};

};

class circular_array
{
public:
    template <typename element_type>
    class allocator
    {
    public:
        using value_type = element_type;

        using pointer_type = element_type*;

        pointer_type allocate(size_t n)
        {
            allocated_blocks_.emplace_back(sizeof(element_type) * n);
            auto& emplaced = allocated_blocks_.back();

            return emplaced.template pointer<pointer_type>();
        }

        void deallocate(pointer_type p, size_t n)
        {
            allocated_blocks_.remove_if([&](auto& tested) { return tested.template pointer<pointer_type>() == p; });
        }

    private:
        list<allocated_block> allocated_blocks_;

    };

    circular_array(size_t capacity)
        :
            capacity_(round_to_page_size<int>(capacity)),
            data_(capacity_)
    {
        data_.reserve();
    }

    py::readonly_memoryview to_memoryview()
    {
        auto first_element = data_.array_one();

        py::buffer_info buffer(
                first_element.first,                  /* Pointer to buffer */
                sizeof(int),                          /* Size of one scalar */
                py::format_descriptor<int>::format(), /* Python struct-style format descriptor */
                data_.size());                        /* Buffer dimensions */

        return py::readonly_memoryview(buffer);
    }

    py::array_t<int> to_array()
    {
        auto numpy = py::module::import("numpy");
        auto array = numpy.attr("array").cast<py::function>();

        auto first_element = data_.array_one();

        py::buffer_info buffer(
                first_element.first,                  /* Pointer to buffer */
                sizeof(int),                          /* Size of one scalar */
                py::format_descriptor<int>::format(), /* Python struct-style format descriptor */
                data_.size());                        /* Buffer dimensions */

        py::memoryview view(buffer);
        auto result = array(view, py::arg("copy") = false);

        return result;
    }

    void append(int value)
    {
        data_.push_back(value);
    }

    int head()
    {
        return data_.front();
    }

    int tail()
    {
        return data_.back();
    }

    void pop()
    {
        data_.pop_front();
    }

    size_t size() const
    {
        return data_.size();
    }

    size_t capacity() const
    {
        return data_.capacity();
    }

private:
    template <typename element_type>
    size_t round_to_page_size(size_t capacity)
    {
        auto page_size = getpagesize();
        if (page_size % sizeof(element_type) != 0)
            throw runtime_error("Elements cannot fit into pages. Please align their size!");

        auto elements_per_page = page_size / sizeof(element_type);
        return capacity * elements_per_page == 0
               ? capacity
               : (capacity / elements_per_page + 1) * elements_per_page;
    }

    size_t capacity_;
    boost::circular_buffer<int, allocator<int>> data_;

};

PYBIND11_MODULE(_circular_buffer_case, m) {

    py::class_<circular_array>(m, "CircularArray")
            .def(py::init<size_t>())
            .def("to_memoryview", &circular_array::to_memoryview)
            .def("append", &circular_array::append)
            .def("pop", &circular_array::pop)
            .def_property_readonly("head", &circular_array::head)
            .def_property_readonly("tail", &circular_array::tail)
            .def_property_readonly("array", &circular_array::to_array)
            .def_property_readonly("size", &circular_array::size)
            .def_property_readonly("capacity", &circular_array::capacity);

}

