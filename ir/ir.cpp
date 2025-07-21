#include "ast.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

PYBIND11_MAKE_OPAQUE(parser::ast::ExprHandle);

namespace IR {

namespace py = pybind11;

namespace ast = parser::ast;

PYBIND11_MODULE(ir_python, m) {
  py::class_<ast::ExprHandle>(m, "ExprHandle")
      .def("get", [](ast::ExprHandle& expr) { return *expr; });

  py::class_<ast::types::Int>(m, "TypeInt")
      .def("type", [](ast::types::Int const&) { return "int"; });

  py::class_<ast::LiteralInt>(m, "LiteralInt")
      .def_readonly("value", &ast::LiteralInt::value);

  py::class_<ast::IdExpr>(m, "IdExpr")
      .def_readwrite("id", &ast::IdExpr::id);

  py::class_<ast::BinOp> bin_op(m, "BinOp");
  bin_op.def_readonly("kind", &ast::BinOp::kind)
      .def_readonly("lhs", &ast::BinOp::lhs)
      .def_readonly("rhs", &ast::BinOp::rhs);

  py::class_<ast::DeclVariable>(m, "DeclVariable")
      .def_readonly("id", &ast::DeclVariable::id)
      .def_readonly("domain", &ast::DeclVariable::domain);

  py::enum_<ast::BinOp::OpKind>(bin_op, "OpKind", "enum.Enum")
      .value("DOTDOT", ast::BinOp::OpKind::DOTDOT)
      .value("NQ", ast::BinOp::OpKind::NQ)
      .export_values();

  py::class_<ast::DeclConst>(m, "DeclConst")
      .def_readonly("id", &ast::DeclConst::id)
      .def_readonly("type", &ast::DeclConst::type)
      .def_readonly("value", &ast::DeclConst::value);

  py::class_<ast::Tree>(m, "Tree")
      .def_readonly("decls", &ast::Tree::decls)
      .def_readonly("constraints", &ast::Tree::constraints);
}

} // namespace IR
