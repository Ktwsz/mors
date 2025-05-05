#include "lib.hpp"

#include <iostream>

#include <minizinc/flattener.hh>
#include <minizinc/model.hh>

namespace frontend {
namespace {
void ind(int const indent) {
  for (int i = 0; i < indent; i += 2)
    std::cout << "| ";
}

struct PrintModelVisitor {
  MiniZinc::Model const &model;
  MiniZinc::EnvI &env;

  static bool enterModel(MiniZinc::Model *) { return true; }
  /// Enter item
  static bool enter(MiniZinc::Item * /*m*/) { return true; }
  /// Visit include item
  void vIncludeI(MiniZinc::IncludeI *include) {
    std::cout << "includeI: " << include->f().c_str() << std::endl;
  }
  /// Visit variable declaration
  void vVarDeclI(MiniZinc::VarDeclI *varDecl) {
    if (!varDecl->loc().filename().empty() &&
        varDecl->loc().filename().endsWith("cumulative.mzn")) {
      match_expr(varDecl->e());
    }
  }
  /// Visit assign item
  void vAssignI(MiniZinc::AssignI * /*ai*/) {
    std::cout << "assignI" << std::endl;
  }
  /// Visit constraint item
  void vConstraintI(MiniZinc::ConstraintI *constraint) {
    std::cout << "constraint expr" << std::endl;
    match_expr(constraint->e());
  }
  /// Visit solve item
  void vSolveI(MiniZinc::SolveI * /*si*/) {
    std::cout << "solveI" << std::endl;
  }
  /// Visit output item
  void vOutputI(MiniZinc::OutputI * /*oi*/) {
    std::cout << "output expr" << std::endl;
  }
  /// Visit function item
  void vFunctionI(MiniZinc::FunctionI *functionI) {
    /*std::cout << "functionI: id=" << functionI->id().c_str() << std::endl;*/
  }

  void print_var_decl(MiniZinc::VarDecl *var_decl, int const indent) {
    ind(indent);
    std::cout << "variable declaration" << std::endl;
    ind(indent + 2);
    std::cout << "id: " << var_decl->id()->v().c_str() << std::endl;
  }

  void print_fn_call(MiniZinc::Call *call, int const indent) {
    auto const functionItem = model.matchFn(env, call, true, false);
    ind(indent);
    std::cout << "function call" << std::endl;
    ind(indent + 2);
    std::cout << "id: " << functionItem->id().c_str() << std::endl;
    if (functionItem->e()) {
      ind(indent + 2);
      std::cout << "function body: " << std::endl;
      match_expr(functionItem->e(), indent + 4);
    }
    std::cout << std::endl;
  }

  void print_let_expr(MiniZinc::Let *let, int const indent) {
    ind(indent);
    std::cout << "let: " << std::endl;
    ind(indent + 2);
    std::cout << "declarations: " << std::endl;
    for (auto e : let->let()) {
      match_expr(e, indent + 4);
    }
    ind(indent);
    std::cout << "in: " << std::endl;
    match_expr(let->in(), indent + 2);
    std::cout << std::endl;
  }

  void match_expr(MiniZinc::Expression *expr, int const indent = 0) {
    switch (MiniZinc::Expression::eid(expr)) {
    case MiniZinc::Expression::E_INTLIT:
      std::cout << "E_INTLIT" << std::endl;
      break;
    case MiniZinc::Expression::E_FLOATLIT:
      std::cout << "E_FLOATLIT" << std::endl;
      break;
    case MiniZinc::Expression::E_SETLIT:
      std::cout << "E_SETLIT" << std::endl;
      break;
    case MiniZinc::Expression::E_BOOLLIT:
      std::cout << "E_BOOLLIT" << std::endl;
      break;
    case MiniZinc::Expression::E_STRINGLIT:
      std::cout << "E_STRINGLIT" << std::endl;
      break;
    case MiniZinc::Expression::E_ID:
      std::cout << "E_ID" << std::endl;
      break;
    case MiniZinc::Expression::E_ANON:
      std::cout << "E_ANON" << std::endl;
      break;
    case MiniZinc::Expression::E_ARRAYLIT:
      std::cout << "E_ARRAYLIT" << std::endl;
      break;
    case MiniZinc::Expression::E_ARRAYACCESS:
      std::cout << "E_ARRAYACCESS" << std::endl;
      break;
    case MiniZinc::Expression::E_FIELDACCESS:
      std::cout << "E_FIELDACCESS" << std::endl;
      break;
    case MiniZinc::Expression::E_COMP:
      std::cout << "E_COMP" << std::endl;
      break;
    case MiniZinc::Expression::E_ITE:
      std::cout << "E_ITE" << std::endl;
      break;
    case MiniZinc::Expression::E_BINOP:
      std::cout << "E_BINOP" << std::endl;
      break;
    case MiniZinc::Expression::E_UNOP:
      std::cout << "E_UNOP" << std::endl;
      break;
    case MiniZinc::Expression::E_CALL: {
      auto *call = MiniZinc::Expression::cast<MiniZinc::Call>(expr);
      print_fn_call(call, indent);
      break;
    }
    case MiniZinc::Expression::E_VARDECL: {
      auto *varDecl = MiniZinc::Expression::cast<MiniZinc::VarDecl>(expr);
      print_var_decl(varDecl, indent);
      break;
    }
    case MiniZinc::Expression::E_LET: {
      auto *let = MiniZinc::Expression::cast<MiniZinc::Let>(expr);
      print_let_expr(let, indent);
      break;
    }
    case MiniZinc::Expression::E_TI:
      std::cout << "E_TI" << std::endl;
      break;
    case MiniZinc::Expression::E_TIID:
      std::cout << "E_TIID" << std::endl;
      break;
    }
  }
};
} // namespace

void say_hello() {
  try {
    auto flt = MiniZinc::Flattener(
        std::cout, std::cerr, "/home/ktwsz/studia/libminizinc/share/minizinc");
    int i = 0;
    std::vector<std::string> arg0 = {"cumulative.mzn"};
    flt.processOption(i, arg0);

    std::vector<std::string> arg1 = {"cumulative.dzn"};
    flt.processOption(i, arg1);

    std::vector<std::string> arg2 = {
        "--instance-check-only", "-I",
        "/home/ktwsz/studia/libminizinc/share/minizinc/solvers/cp-sat"};
    flt.processOption(i, arg2);

    std::vector<std::string> arg3 = {
        "-I", "/home/ktwsz/studia/libminizinc/share/minizinc/solvers/cp-sat"};
    flt.processOption(i, arg3);

    flt.flatten("", "stdin");

    auto &model = *flt.getEnv()->model();

    std::cout << "--- AST ---" << std::endl;

    PrintModelVisitor vis{model, flt.getEnv()->envi()};
    MiniZinc::iter_items<PrintModelVisitor>(vis, &model);

    std::cout << "-----------" << std::endl;
  } catch (MiniZinc::Exception const &e) {
    std::cout << e.msg() << std::endl;
  }
}

} // namespace frontend
