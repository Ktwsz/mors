#include "ast.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace IR {

namespace py = pybind11;

// struct LiteralInt {
//   long long value;
// };
//
// using Expr = std::variant<LiteralInt>;
//
// struct Domain {
//   Expr lower, upper;
// };
//
// namespace types {
// struct Int {};
// } // namespace types
//
// using Type = std::variant<types::Int>;
//
//
// struct DeclVariable {
//   std::string id;
//   Type type;
//
//   std::optional<Domain> domain;
// };
//
// struct DeclConst {
//   std::string id;
//   Type type;
//
//   Expr value;
// };
//
// using ASTNode = std::variant<DeclVariable, DeclConst>;
//
// struct Tree {
//     std::vector<ASTNode> decls;
// };

namespace ast = parser::ast;

PYBIND11_MODULE(ir_python, m) {
  py::class_<ast::DeclVariable>(m, "DeclVariable")
      .def("type", [](ast::ASTNode const& node) { return "var"; })
      .def_readwrite("id", &ast::DeclVariable::id);

  py::class_<ast::DeclConst>(m, "DeclConst")
      .def("type", [](ast::ASTNode const& node) { return "const"; })
      .def_readwrite("id", &ast::DeclConst::id);

  // py::class_<ast::ASTNode>(m, "ASTNode");

  py::class_<ast::Tree>(m, "Tree").def_readwrite("decls", &ast::Tree::decls);
}

} // namespace IR
