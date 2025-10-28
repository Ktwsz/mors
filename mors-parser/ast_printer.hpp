#pragma once

#include <minizinc/model.hh>

namespace parser {

struct PrintModelVisitor {
  MiniZinc::Model const& model;
  MiniZinc::EnvI& env;

  std::string input_model_path;

  void print_type(MiniZinc::Type const& type, int indent);

  void print_var_decl(MiniZinc::VarDecl* var_decl, int indent);

  void print_type_inst(MiniZinc::TypeInst* var_decl, int indent);

  void print_fn_call(MiniZinc::Call* call, int indent);

  void print_ite(MiniZinc::ITE* ite, int indent);

  void print_let_expr(MiniZinc::Let* let, int indent);

  void print_int_lit(MiniZinc::IntLit* int_lit, int indent);

  void print_set_lit(MiniZinc::SetLit* int_lit, int indent);

  void print_float_lit(MiniZinc::FloatLit* float_lit, int indent);

  void print_bin_op(MiniZinc::BinOp* bin_op, int indent);

  void print_un_op(MiniZinc::UnOp* un_op, int indent);

  void print_comprehension(MiniZinc::Comprehension* comp, int indent);

  void print_array_lit(MiniZinc::ArrayLit* array_lit, int indent);

  void print_array_access(MiniZinc::ArrayAccess* array_access, int indent);

  void print_solve_type(MiniZinc::SolveI* solve_item);

  void match_expr(MiniZinc::Expression* expr, int indent = 0);
};

} // namespace parser
