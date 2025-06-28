#pragma once

#include "ir.hpp"

#include <minizinc/model.hh>

namespace parser {

struct PrintModelVisitor {
  MiniZinc::Model const& model;
  MiniZinc::EnvI& env;
  IR::Data& data;

  std::string input_model_path;

  static bool enterModel(MiniZinc::Model*);
  static bool enter(MiniZinc::Item* /*m*/);

  void vIncludeI(MiniZinc::IncludeI* include);
  void vVarDeclI(MiniZinc::VarDeclI* varDecl);

  void vAssignI(MiniZinc::AssignI* /*ai*/);

  void vConstraintI(MiniZinc::ConstraintI* constraint);

  void vSolveI(MiniZinc::SolveI* /*si*/);

  void vOutputI(MiniZinc::OutputI* /*oi*/);

  void vFunctionI(MiniZinc::FunctionI* functionI);

  void print_var_decl(MiniZinc::VarDecl* var_decl, int const indent);

  void print_type_inst(MiniZinc::TypeInst* var_decl, int const indent);

  void print_fn_call(MiniZinc::Call* call, int const indent);

  void print_ite(MiniZinc::ITE* ite, int const indent);

  void print_let_expr(MiniZinc::Let* let, int const indent);

  void print_int_lit(MiniZinc::IntLit* int_lit, int const indent);

  void print_bin_op(MiniZinc::BinOp* bin_op, int const indent);

  void match_expr(MiniZinc::Expression* expr, int const indent = 0);
};

} // namespace parser
