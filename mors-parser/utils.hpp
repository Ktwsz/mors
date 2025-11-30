#pragma once

#include "ast.hpp"

namespace parser::utils {

template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};

auto is_type_set(ast::Type const& t) -> bool;

auto expr_type(ast::Expr const& expr) -> ast::Type;
auto is_expr_var(ast::Expr const& expr) -> bool;

auto var_id(ast::VarDecl const& var) -> std::string;
auto var_value(ast::VarDecl const& var) -> ast::ExprHandle;
auto is_var_global(ast::VarDecl const&) -> bool;
auto var_type(ast::VarDecl const&) -> ast::Type;
auto is_var_var(ast::VarDecl const&) -> bool;

auto is_unsupported_var_type(ast::Type const&) -> bool;
auto type_to_string(ast::Type const&) -> std::string;
auto outer_type_to_string(ast::Type const& type) -> std::string;

} // namespace parser::utils
