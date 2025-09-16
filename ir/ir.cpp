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

  // TODO: floats not supported for now
  py::class_<ast::types::Float>(m, "TypeFloat")
      .def("type", [](ast::types::Float const&) { return "float"; });

  py::class_<ast::types::Bool>(m, "TypeBool")
      .def("type", [](ast::types::Bool const&) { return "bool"; });

  py::class_<ast::types::String>(m, "TypeString")
      .def("type", [](ast::types::String const&) { return "string"; });

  py::class_<ast::types::IntSet>(m, "IntSet")
      .def("type", [](ast::types::IntSet const&) { return "int_set"; });

  py::class_<ast::types::FloatSet>(m, "FloatSet")
      .def("type", [](ast::types::FloatSet const&) { return "float_set"; });

  py::class_<ast::types::BoolSet>(m, "BoolSet")
      .def("type", [](ast::types::BoolSet const&) { return "bool_set"; });

  py::class_<ast::types::UnspecifiedSet>(m, "UnspecifiedSet")
      .def("type",
           [](ast::types::UnspecifiedSet const&) { return "unspecified_set"; });

  py::class_<ast::types::Array>(m, "Array")
      .def("type", [](ast::types::Array const&) { return "array"; })
      .def_readonly("dims", &ast::types::Array::dims);

  py::class_<ast::LiteralBool>(m, "LiteralBool")
      .def_readonly("value", &ast::LiteralBool::value)
      .def_readonly("expr_type", &ast::LiteralBool::expr_type);

  py::class_<ast::LiteralInt>(m, "LiteralInt")
      .def_readonly("value", &ast::LiteralInt::value)
      .def_readonly("expr_type", &ast::LiteralInt::expr_type);

  // TODO: floats not supported for now
  py::class_<ast::LiteralFloat>(m, "LiteralFloat")
      .def_readonly("value", &ast::LiteralFloat::value)
      .def_readonly("expr_type", &ast::LiteralFloat::expr_type);

  py::class_<ast::LiteralString>(m, "LiteralString")
      .def_readonly("value", &ast::LiteralString::value)
      .def_readonly("expr_type", &ast::LiteralString::expr_type);

  py::class_<ast::LiteralArray>(m, "LiteralArray")
      .def_readonly("value", &ast::LiteralArray::value)
      .def_readonly("expr_type", &ast::LiteralArray::expr_type);

  py::class_<ast::LiteralSet>(m, "LiteralSet")
      .def_readonly("value", &ast::LiteralSet::value)
      .def_readonly("expr_type", &ast::LiteralSet::expr_type);

  py::class_<ast::ArrayAccess>(m, "ArrayAccess")
      .def_readonly("arr", &ast::ArrayAccess::arr)
      .def_readonly("indexes", &ast::ArrayAccess::indexes)
      .def_readonly("expr_type", &ast::ArrayAccess::expr_type);

  py::class_<ast::Iterator>(m, "Iterator")
      .def_readonly("variable", &ast::Iterator::variable)
      .def_readonly("in_expr", &ast::Iterator::in);

  py::class_<ast::Generator>(m, "Generator");

  py::class_<ast::Comprehension>(m, "Comprehension")
      .def_readonly("body", &ast::Comprehension::body)
      .def_readonly("generators", &ast::Comprehension::generators)
      .def_readonly("expr_type", &ast::Comprehension::expr_type);

  py::class_<ast::IfThenElse>(m, "IfThenElse")
      .def_readonly("if_then", &ast::IfThenElse::if_then)
      .def_readonly("else_expr", &ast::IfThenElse::else_expr)
      .def_readonly("expr_type", &ast::IfThenElse::expr_type);

  py::class_<ast::IdExpr>(m, "IdExpr")
      .def_readwrite("id", &ast::IdExpr::id)
      .def_readwrite("is_global", &ast::IdExpr::is_global)
      .def_readwrite("is_var", &ast::IdExpr::is_var)
      .def_readonly("expr_type", &ast::IdExpr::expr_type);

  py::class_<ast::BinOp> bin_op(m, "BinOp");
  bin_op.def_readonly("kind", &ast::BinOp::kind)
      .def_readonly("lhs", &ast::BinOp::lhs)
      .def_readonly("rhs", &ast::BinOp::rhs)
      .def_readonly("expr_type", &ast::BinOp::expr_type);

  py::enum_<ast::BinOp::OpKind>(bin_op, "OpKind", "enum.Enum")
      .value("PLUS", ast::BinOp::OpKind::PLUS)
      .value("MINUS", ast::BinOp::OpKind::MINUS)
      .value("MULT", ast::BinOp::OpKind::MULT)
      .value("IDIV", ast::BinOp::OpKind::IDIV)
      .value("MOD", ast::BinOp::OpKind::MOD)
      .value("DOTDOT", ast::BinOp::OpKind::DOTDOT)
      .value("EQ", ast::BinOp::OpKind::EQ)
      .value("NQ", ast::BinOp::OpKind::NQ)
      .value("GQ", ast::BinOp::OpKind::GQ)
      .value("GR", ast::BinOp::OpKind::GR)
      .value("LE", ast::BinOp::OpKind::LE)
      .value("LQ", ast::BinOp::OpKind::LQ)
      .value("PLUSPLUS", ast::BinOp::OpKind::PLUSPLUS)
      .value("AND", ast::BinOp::OpKind::AND)
      .value("OR", ast::BinOp::OpKind::OR)
      .export_values();

  py::class_<ast::UnaryOp> un_op(m, "UnaryOp");
  un_op.def_readonly("kind", &ast::UnaryOp::kind)
      .def_readonly("expr", &ast::UnaryOp::expr)
      .def_readonly("expr_type", &ast::UnaryOp::expr_type);

  py::enum_<ast::UnaryOp::OpKind>(un_op, "OpKind", "enum.Enum")
      .value("NOT", ast::UnaryOp::OpKind::NOT)
      .value("PLUS", ast::UnaryOp::OpKind::PLUS)
      .value("MINUS", ast::UnaryOp::OpKind::MINUS)
      .export_values();

  py::class_<ast::Call>(m, "Call")
      .def_readonly("id", &ast::Call::id)
      .def_readonly("args", &ast::Call::args)
      .def_readonly("expr_type", &ast::Call::expr_type);

  py::class_<ast::DeclVariable>(m, "DeclVariable")
      .def_readonly("id", &ast::DeclVariable::id)
      .def_readonly("type", &ast::DeclVariable::var_type)
      .def_readonly("domain", &ast::DeclVariable::domain);

  py::class_<ast::DeclConst>(m, "DeclConst")
      .def_readonly("id", &ast::DeclConst::id)
      .def_readonly("type", &ast::DeclConst::type)
      .def_readonly("value", &ast::DeclConst::value);

  py::class_<ast::solve_type::Sat> SolveTypeSat(m, "SolveTypeSat");
  py::class_<ast::solve_type::Min>(m, "SolveTypeMin")
      .def_readonly("expr", &ast::solve_type::Min::expr);
  py::class_<ast::solve_type::Max>(m, "SolveTypeMax")
      .def_readonly("expr", &ast::solve_type::Max::expr);

  py::class_<ast::Function>(m, "Function")
      .def_readonly("id", &ast::Function::id)
      .def_readonly("params", &ast::Function::params)
      .def_readonly("body", &ast::Function::body);

  py::class_<ast::Tree>(m, "Tree")
      .def_readonly("decls", &ast::Tree::decls)
      .def_readonly("constraints", &ast::Tree::constraints)
      .def_readonly("solve_type", &ast::Tree::solve_type)
      .def_readonly("output", &ast::Tree::output)
      .def_readonly("functions", &ast::Tree::functions);
}

} // namespace IR
