#pragma once

#include "ir.hpp"

#include <minizinc/model.hh>

namespace parser {

struct PrintModelVisitor {
  MiniZinc::Model const &model;
  MiniZinc::EnvI &env;
  IR::Data &data;


  static bool enterModel(MiniZinc::Model *);
  static bool enter(MiniZinc::Item * /*m*/);

  void vIncludeI(MiniZinc::IncludeI *include);
  void vVarDeclI(MiniZinc::VarDeclI *varDecl);

  void vAssignI(MiniZinc::AssignI * /*ai*/);

  void vConstraintI(MiniZinc::ConstraintI *constraint);

  void vSolveI(MiniZinc::SolveI * /*si*/);

  void vOutputI(MiniZinc::OutputI * /*oi*/);

  void vFunctionI(MiniZinc::FunctionI *functionI);

  void print_var_decl(MiniZinc::VarDecl *var_decl, int const indent);

  void print_fn_call(MiniZinc::Call *call, int const indent);

  void print_ite(MiniZinc::ITE *ite, int const indent);

  void print_let_expr(MiniZinc::Let *let, int const indent);

  void match_expr(MiniZinc::Expression *expr, int const indent = 0);
};

} // namespace parser
