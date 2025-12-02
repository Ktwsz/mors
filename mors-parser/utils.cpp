#include "utils.hpp"

#include <cassert>
#include <string>
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

struct SetTemplate {
  template <typename T> bool operator()(ast::types::Set<T> const&) {
    return true;
  }
};

} // namespace

auto is_type_set(ast::Type const& t) -> bool {
  return std::visit(
      overloaded{SetTemplate{}, [](auto const& _) { return false; }}, t);
}

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

auto is_unsupported_var_type(ast::Type const& type) -> bool {
  return std::visit(
      utils::overloaded{[](ast::types::Int const&) { return false; },
                        [](ast::types::Bool const&) { return false; },
                        [](ast::types::Array const& arr) {
                          return is_unsupported_var_type(*arr.inner_type);
                        },
                        [](auto const&) { return true; }},
      type);
}

auto type_to_string(ast::Type const& type) -> std::string {
  using std::operator""s;
  return std::visit(
      utils::overloaded{
          [](ast::types::Int const&) { return "int"s; },
          [](ast::types::Bool const&) { return "boolean"s; },
          [](ast::types::Float const&) { return "float"s; },
          [](ast::types::String const&) { return "string"s; },
          [](ast::types::Unspecified const&) { return "unspecified"s; },
          [](ast::types::IntSet const&) { return "set of int"s; },
          [](ast::types::FloatSet const&) { return "set of float"s; },
          [](ast::types::BoolSet const&) { return "set of bool"s; },
          [](ast::types::UnspecifiedSet const&) {
            return "set of unspecified"s;
          },
          [](ast::types::Array const& arr) {
            return "array of " + type_to_string(*arr.inner_type);
          },
      },
      type);
}

}; // namespace parser::utils
