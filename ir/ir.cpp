#include "ir.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace IR {

namespace py = pybind11;

PYBIND11_MODULE(ir_python, m) {
  py::class_<Data>(m, "Data").def_readwrite("ids", &Data::ids);
}

} // namespace IR
