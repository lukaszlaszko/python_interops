#pragma once

#include <pybind11/pybind11.h>


namespace pybind11 {

class readonly_memoryview : public object {
public:
    explicit readonly_memoryview(const buffer_info& info);

    PYBIND11_OBJECT_CVT(readonly_memoryview, object, PyMemoryView_Check, PyMemoryView_FromObject)
};

}

#include "readonly_memoryview.ipp"
