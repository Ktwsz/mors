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
  ast::VariableMap variable_map{
      {"mzn_min_version_required",
       {ast::DeclConst{

           .id = "mzn_min_version_required",
           .type = ast::types::Int{},
           .is_global = true,
           .value = std::make_shared<ast::Expr>(ast::LiteralInt{0})}}},
      {"mzn_max_version_required",
       {ast::DeclConst{

           .id = "mzn_max_version_required",
           .type = ast::types::Int{},
           .is_global = true,
           .value = std::make_shared<ast::Expr>(ast::LiteralInt{0})}}}
  };

  std::string input_model_path;

  size_t let_in_ctr = 0;

  auto map(MiniZinc::VarDecl*, bool is_global) -> std::optional<ast::VarDecl>;
  auto map(MiniZinc::Expression*) -> std::optional<ast::ExprHandle>;
  auto map(MiniZinc::SolveI*) -> ast::SolveType;

private:
  auto map(MiniZinc::SetLit*) -> ast::ExprHandle;
  auto map(MiniZinc::TypeInst* type_inst) -> ast::Type;
  auto map(MiniZinc::Comprehension*) -> ast::Comprehension;
  auto map(MiniZinc::Call*) -> ast::ExprHandle;
  auto map(MiniZinc::BinOp*) -> ast::ExprHandle;
  auto map(MiniZinc::UnOp*) -> ast::ExprHandle;
  auto map(MiniZinc::ITE*) -> ast::ExprHandle;
  auto map(MiniZinc::Let*) -> ast::ExprHandle;
  void save(MiniZinc::FunctionI*);

  auto handle_const_decl(MiniZinc::VarDecl* var_decl) -> ast::VarDecl;
  auto handle_var_decl(MiniZinc::VarDecl* var_decl) -> ast::VarDecl;
  auto find_objective_expr() -> ast::ExprHandle;
};

} // namespace parser
