#include <pybind11/pybind11.h>
#include <pybind11/readonly_memoryview.hpp>

#include <vector>

using namespace std;
namespace py = pybind11;


class custom_array
{
public:
    custom_array(size_t length)
        : data_(length, 0)
    {

    }

    py::buffer_info to_buffer()
    {
        return py::buffer_info(
                data_.data(),                         /* Pointer to buffer */
                sizeof(int),                          /* Size of one scalar */
                py::format_descriptor<int>::format(), /* Python struct-style format descriptor */
                data_.size()                          /* Buffer dimensions */
        );
    }

    int get(size_t index) const
    {
        return data_.at(index);
    }

    void set(size_t index, int value)
    {
        data_.at(index) = value;
    }

    size_t size() const
    {
        return data_.size();
    }

    py::readonly_memoryview to_readonly()
    {
        return py::readonly_memoryview(to_buffer());
    }

private:
    std::vector<int> data_;

};

PYBIND11_MODULE(_array_case, m) {

    py::class_<custom_array>(m, "Array", py::buffer_protocol())
            .def(py::init<size_t>())
            .def_buffer(&custom_array::to_buffer)
            .def("get", &custom_array::get)
            .def("set", &custom_array::set)
            .def("to_readonly", &custom_array::to_readonly)
            .def_property_readonly("size", &custom_array::size);

}
