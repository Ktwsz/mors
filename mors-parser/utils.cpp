#include "utils.hpp"

#include <string>
#include <cassert>
#include <variant>

namespace parser::utils {

namespace {
template <typename T>
concept HasType = requires(T t) {
  { t.expr_type } -> std::same_as<ast::Type&>;
};
template <typename T>
concept HasIsVar = requires(T t) {
  { t.is_var } -> std::same_as<bool&>;
};
} // namespace

auto expr_type(ast::Expr const& expr) -> ast::Type {
  return std::visit(
      overloaded{[](HasType auto const& t) { return t.expr_type; }}, expr);
}

auto is_expr_var(ast::Expr const& expr) -> bool {
  return std::visit(overloaded{[](HasIsVar auto const& t) { return t.is_var; }},
                    expr);
}

auto var_id(ast::VarDecl const& var) -> std::string {
  return std::visit(overloaded{
                        [](ast::DeclVariable const& var) { return var.id; },
                        [](ast::DeclConst const& var) { return var.id; },
                    },
                    var);
}

auto var_value(ast::VarDecl const& var) -> ast::ExprHandle {
  return std::visit(overloaded{
                        [](ast::DeclVariable const& var) {
                          assert(var.value);
                          return *var.value;
                        },
                        [](ast::DeclConst const& var) {
                          assert(var.value);
                          return *var.value;
                        },
                    },
                    var);
}

auto is_var_global(ast::VarDecl const& var) -> bool {
  return std::visit(
      overloaded{
          [](ast::DeclVariable const& var) { return var.is_global; },
          [](ast::DeclConst const& var) { return var.is_global; },
      },
      var);
}

auto var_type(ast::VarDecl const& var) -> ast::Type {
  return std::visit(
      overloaded{
          [](ast::DeclVariable const& var) { return var.var_type; },
          [](ast::DeclConst const& var) { return var.type; },
      },
      var);
}

auto is_var_var(ast::VarDecl const& var) -> bool {
  return std::visit(overloaded{
                        [](ast::DeclVariable const&) { return true; },
                        [](ast::DeclConst const&) { return false; },
                    },
                    var);
}

}; // namespace parser::utils
