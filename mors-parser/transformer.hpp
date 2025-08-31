#pragma once

#include "ast.hpp"

#include <minizinc/ast.hh>
#include <minizinc/model.hh>

#include <optional>

namespace parser {

struct Transformer {
  MiniZinc::Model& model;
  MiniZinc::EnvI& env;

  ast::FunctionMap& functions;

  std::string input_model_path;

  auto map(MiniZinc::VarDecl*) -> std::optional<ast::VarDecl>;
  auto map(MiniZinc::Expression*) -> std::optional<ast::ExprHandle>;
  auto map(MiniZinc::SolveI*) -> ast::SolveType;

private:
  auto map(MiniZinc::SetLit*) -> ast::ExprHandle;
  auto map(MiniZinc::TypeInst* type_inst) -> ast::Type;
  auto map(MiniZinc::Comprehension*) -> ast::Comprehension;
  auto map(MiniZinc::BinOp*) -> ast::ExprHandle;
  void save(MiniZinc::FunctionI*);

  auto handle_const_decl(MiniZinc::VarDecl* var_decl) -> ast::VarDecl;
  auto handle_var_decl(MiniZinc::VarDecl* var_decl) -> ast::VarDecl;
  auto find_objective_expr() -> ast::ExprHandle;
};

} // namespace parser
