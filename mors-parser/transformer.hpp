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

  auto map(MiniZinc::VarDecl*) -> std::optional<ast::VarDecl>;
  auto map(MiniZinc::Expression*) -> std::optional<ast::ExprHandle>;
  auto map(MiniZinc::BinOp*) -> ast::ExprHandle;

private:
  auto handle_const_decl(MiniZinc::VarDecl* var_decl) -> ast::VarDecl;
  auto handle_var_decl(MiniZinc::VarDecl* var_decl) -> ast::VarDecl;
};

} // namespace parser
