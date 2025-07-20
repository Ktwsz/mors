#include "ast.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace IR {

namespace py = pybind11;

namespace ast = parser::ast;

PYBIND11_MODULE(ir_python, m) {
  py::class_<ast::LiteralInt>(m, "LiteralInt")
      .def_readwrite("value", &ast::LiteralInt::value);

  py::class_<ast::IdExpr>(m, "IdExpr").def_readwrite("id", &ast::IdExpr::id);

  py::class_<ast::Domain>(m, "Domain")
      .def_readwrite("lower", &ast::Domain::lower)
      .def_readwrite("upper", &ast::Domain::upper);

  py::class_<ast::DeclVariable>(m, "DeclVariable")
      .def_readwrite("id", &ast::DeclVariable::id)
      .def_readwrite("domain", &ast::DeclVariable::domain);

  py::class_<ast::types::Int>(m, "TypeInt")
      .def("type", [](ast::types::Int const&) { return "int"; });

  py::class_<ast::DeclConst>(m, "DeclConst")
      .def_readwrite("id", &ast::DeclConst::id)
      .def_readwrite("type", &ast::DeclConst::type)
      .def_readwrite("value", &ast::DeclConst::value);

  py::class_<ast::Tree>(m, "Tree").def_readwrite("decls", &ast::Tree::decls);
}

} // namespace IR
