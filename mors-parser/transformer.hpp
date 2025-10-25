#pragma once

#include "ast.hpp"
#include "parser_opts.hpp"

#include <minizinc/ast.hh>
#include <minizinc/model.hh>

#include <optional>

namespace parser {

struct Transformer {
  MiniZinc::Model& model;
  MiniZinc::EnvI& env;

  ast::FunctionMap& functions;
  struct Stack {
    struct Scope {
      std::vector<std::string> decls;
      ast::VariableMap& map;

      Scope(ast::VariableMap& pMap);
      void add(ast::VarDecl const& var);
      ~Scope();
    };

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

    auto scope() -> Scope;
  };

  Stack stack;

  ParserOpts const& opts;

  size_t let_in_ctr = 0;

  auto map(MiniZinc::VarDecl*, bool is_global, bool check_id)
      -> std::optional<ast::VarDecl>;
  auto map(MiniZinc::Expression*) -> ast::Expr;
  auto map_ptr(MiniZinc::Expression*) -> ast::ExprHandle;
  auto map(MiniZinc::SolveI*) -> ast::SolveType;

private:
  auto map(MiniZinc::ArrayLit*) -> ast::Expr;
  auto map(MiniZinc::SetLit*) -> ast::Expr;
  auto map(MiniZinc::TypeInst*) -> ast::Type;
  auto map(MiniZinc::Type const&) -> ast::Type;
  auto map(MiniZinc::Comprehension*) -> ast::Comprehension;
  auto map(MiniZinc::Call*) -> ast::Expr;
  auto map(MiniZinc::ArrayAccess*) -> ast::Expr;
  auto map(MiniZinc::BinOp*) -> ast::Expr;
  auto map(MiniZinc::UnOp*) -> ast::Expr;
  auto map(MiniZinc::ITE*) -> ast::Expr;
  auto map(MiniZinc::Let*) -> ast::Expr;
  void save(MiniZinc::FunctionI*);

  auto handle_const_decl(MiniZinc::VarDecl* var_decl) -> ast::VarDecl;
  auto handle_var_decl(MiniZinc::VarDecl* var_decl) -> ast::VarDecl;
  auto find_objective_expr() -> ast::Expr;
};

} // namespace parser
