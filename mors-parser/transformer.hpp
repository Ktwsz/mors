#pragma once

#include "ast.hpp"

#include <minizinc/ast.hh>
#include <minizinc/model.hh>

#include <optional>

namespace parser {

struct Transformer {
  MiniZinc::Model const& model;
  MiniZinc::EnvI& env;

  std::string input_model_path;

  auto map(MiniZinc::VarDecl*) -> std::optional<ast::ASTNode>;
  auto map(MiniZinc::Expression*) -> ast::Expr;

private:
  auto handle_const_decl(MiniZinc::VarDecl* var_decl) -> ast::ASTNode;
  auto handle_var_decl(MiniZinc::VarDecl* var_decl) -> ast::ASTNode;
  auto handle_domain(MiniZinc::BinOp* bin_op) -> ast::Domain;
  auto handle_domain(MiniZinc::Expression* expr) -> ast::Domain;
};

} // namespace parser
