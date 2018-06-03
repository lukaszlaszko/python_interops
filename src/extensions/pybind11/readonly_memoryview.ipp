#pragma once

#include "readonly_memoryview.hpp"


namespace pybind11 {

readonly_memoryview::readonly_memoryview(const buffer_info& info)
{
    static Py_buffer buf { };
    // Py_buffer uses signed sizes, strides and shape!..
    static std::vector<Py_ssize_t> py_strides { };
    static std::vector<Py_ssize_t> py_shape { };
    buf.buf = info.ptr;
    buf.itemsize = info.itemsize;
    buf.format = const_cast<char *>(info.format.c_str());
    buf.ndim = (int) info.ndim;
    buf.len = info.size;
    py_strides.clear();
    py_shape.clear();
    for (size_t i = 0; i < (size_t) info.ndim; ++i) {
        py_strides.push_back(info.strides[i]);
        py_shape.push_back(info.shape[i]);
    }
    buf.strides = py_strides.data();
    buf.shape = py_shape.data();
    buf.suboffsets = nullptr;
    buf.readonly = true;
    buf.internal = nullptr;

    m_ptr = PyMemoryView_FromBuffer(&buf);
    if (!m_ptr)
        pybind11_fail("Unable to create memoryview from buffer descriptor");
}

}

