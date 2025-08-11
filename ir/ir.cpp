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

  py::class_<ast::types::Float>(m, "TypeFloat")
      .def("type", [](ast::types::Float const&) { return "float"; });

  py::class_<ast::types::Bool>(m, "TypeBool")
      .def("type", [](ast::types::Bool const&) { return "bool"; });

  py::class_<ast::types::IntSet>(m, "IntSet")
      .def("type", [](ast::types::IntSet const&) { return "int_set"; });

  py::class_<ast::types::FloatSet>(m, "FloatSet")
      .def("type", [](ast::types::FloatSet const&) { return "float_set"; });

  py::class_<ast::types::BoolSet>(m, "BoolSet")
      .def("type", [](ast::types::BoolSet const&) { return "bool_set"; });

  py::class_<ast::types::Array>(m, "Array")
      .def("type", [](ast::types::Array const&) { return "array"; })
      .def_readonly("dims", &ast::types::Array::dims);

  py::class_<ast::LiteralInt>(m, "LiteralInt")
      .def_readonly("value", &ast::LiteralInt::value);

  py::class_<ast::LiteralFloat>(m, "LiteralFloat")
      .def_readonly("value", &ast::LiteralFloat::value);

  py::class_<ast::LiteralString>(m, "LiteralString")
      .def_readonly("value", &ast::LiteralString::value);

  py::class_<ast::LiteralArray>(m, "LiteralArray")
      .def_readonly("value", &ast::LiteralArray::value);

  py::class_<ast::IdExpr>(m, "IdExpr").def_readwrite("id", &ast::IdExpr::id);

  py::class_<ast::BinOp> bin_op(m, "BinOp");
  bin_op.def_readonly("kind", &ast::BinOp::kind)
      .def_readonly("lhs", &ast::BinOp::lhs)
      .def_readonly("rhs", &ast::BinOp::rhs);

  py::class_<ast::Call>(m, "Call")
      .def_readonly("id", &ast::Call::id)
      .def_readonly("args", &ast::Call::args);

  py::class_<ast::DeclVariable>(m, "DeclVariable")
      .def_readonly("id", &ast::DeclVariable::id)
      .def_readonly("type", &ast::DeclVariable::var_type)
      .def_readonly("domain", &ast::DeclVariable::domain);

  py::enum_<ast::BinOp::OpKind>(bin_op, "OpKind", "enum.Enum")
      .value("PLUS", ast::BinOp::OpKind::PLUS)
      .value("MINUS", ast::BinOp::OpKind::MINUS)
      .value("MULT", ast::BinOp::OpKind::MULT)
      .value("DIV", ast::BinOp::OpKind::DIV)
      .value("DOTDOT", ast::BinOp::OpKind::DOTDOT)
      .value("EQ", ast::BinOp::OpKind::EQ)
      .value("NQ", ast::BinOp::OpKind::NQ)
      .value("PLUSPLUS", ast::BinOp::OpKind::PLUSPLUS)
      .export_values();

  py::class_<ast::DeclConst>(m, "DeclConst")
      .def_readonly("id", &ast::DeclConst::id)
      .def_readonly("type", &ast::DeclConst::type)
      .def_readonly("value", &ast::DeclConst::value);

  py::enum_<ast::SolveType>(m, "SolveType", "enum.Enum")
      .value("MIN", ast::SolveType::MIN)
      .value("MAX", ast::SolveType::MAX)
      .value("SAT", ast::SolveType::SAT)
      .export_values();

  py::class_<ast::Tree>(m, "Tree")
      .def_readonly("decls", &ast::Tree::decls)
      .def_readonly("constraints", &ast::Tree::constraints)
      .def_readonly("solve_type", &ast::Tree::solve_type)
      .def_readonly("output", &ast::Tree::output);
}

} // namespace IR
