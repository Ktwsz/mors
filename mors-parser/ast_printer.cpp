#include "ast_printer.hpp"

#include <fmt/base.h>
#include <fmt/format.h>

namespace parser {
namespace {
void ind(int const indent) {
  for (int i = 0; i < indent; i += 2)
    fmt::print("| ");
}
} // namespace

bool PrintModelVisitor::enterModel(MiniZinc::Model *) { return true; }

bool PrintModelVisitor::enter(MiniZinc::Item * /*m*/) { return true; }

void PrintModelVisitor::vIncludeI(MiniZinc::IncludeI *include) {
  fmt::println("includeI: {}", include->f().c_str());
}

void PrintModelVisitor::vVarDeclI(MiniZinc::VarDeclI *varDecl) {
  // if (varDecl->loc().filename().endsWith(input_model_path)) {
  match_expr(varDecl->e());
  // }
}

void PrintModelVisitor::vAssignI(MiniZinc::AssignI * /*ai*/) {
  fmt::println("assignI");
}

void PrintModelVisitor::vConstraintI(MiniZinc::ConstraintI *constraint) {
  fmt::println("constraint expr");
  match_expr(constraint->e());
}

void PrintModelVisitor::vSolveI(MiniZinc::SolveI * /*si*/) {
  fmt::println("solveI");
}

void PrintModelVisitor::vOutputI(MiniZinc::OutputI * /*oi*/) {
  fmt::println("output expr");
}

void PrintModelVisitor::vFunctionI(MiniZinc::FunctionI *functionI) {}

void PrintModelVisitor::print_var_decl(MiniZinc::VarDecl *var_decl,
                                       int const indent) {
  ind(indent);
  fmt::println("variable declaration");

  ind(indent + 2);
  fmt::println("id: {}", var_decl->id()->v().c_str());

  ind(indent + 2);
  if (var_decl->item()->loc().filename().empty())
    fmt::println("loc: empty");
  else
    fmt::println("loc: {}", var_decl->item()->loc().filename().empty());

  // if (varDecl->loc().filename().endsWith(input_model_path)) {
  data.ids.push_back(var_decl->id()->v().c_str());
}

void PrintModelVisitor::print_fn_call(MiniZinc::Call *call, int const indent) {
  auto const functionItem = model.matchFn(env, call, true, false);

  ind(indent);
  fmt::println("function call");

  ind(indent + 2);
  fmt::println("id: {}", functionItem->id().c_str());

  if (functionItem->e()) {
    ind(indent + 2);
    fmt::println("function body: ");

    match_expr(functionItem->e(), indent + 4);
  }
  fmt::print("\n");
}

void PrintModelVisitor::print_ite(MiniZinc::ITE *ite, int const indent) {
  for (unsigned int i = 0; i < ite->size(); i++) {
    ind(indent);
    fmt::print("if-condition: ");
    match_expr(ite->ifExpr(i), indent + 2);

    ind(indent);
    fmt::print("if-body: ");
    match_expr(ite->thenExpr(i), indent + 2);
  }

  ind(indent);
  fmt::print("else: ");
  match_expr(ite->elseExpr(), indent + 2);
}

void PrintModelVisitor::print_let_expr(MiniZinc::Let *let, int const indent) {
  ind(indent);
  fmt::println("let: ");

  ind(indent + 2);
  fmt::println("declarations");

  for (auto e : let->let()) {
    match_expr(e, indent + 4);
  }

  ind(indent);
  fmt::println("in: ");

  match_expr(let->in(), indent + 2);
  fmt::print("\n");
}

void PrintModelVisitor::match_expr(MiniZinc::Expression *expr,
                                   int const indent) {
  switch (MiniZinc::Expression::eid(expr)) {
  case MiniZinc::Expression::E_INTLIT:
    fmt::println("E_INTLIT");
    break;
  case MiniZinc::Expression::E_FLOATLIT:
    fmt::println("E_FLOATLIT");
    break;
  case MiniZinc::Expression::E_SETLIT:
    fmt::println("E_SETLIT");
    break;
  case MiniZinc::Expression::E_BOOLLIT:
    fmt::println("E_BOOLLIT");
    break;
  case MiniZinc::Expression::E_STRINGLIT:
    fmt::println("E_STRINGLIT");
    break;
  case MiniZinc::Expression::E_ID:
    fmt::println("E_ID");
    break;
  case MiniZinc::Expression::E_ANON:
    fmt::println("E_ANON");
    break;
  case MiniZinc::Expression::E_ARRAYLIT:
    fmt::println("E_ARRAYLIT");
    break;
  case MiniZinc::Expression::E_ARRAYACCESS:
    fmt::println("E_ARRAYACCESS");
    break;
  case MiniZinc::Expression::E_FIELDACCESS:
    fmt::println("E_FIELDACCESS");
    break;
  case MiniZinc::Expression::E_COMP:
    fmt::println("E_COMP");
    break;
  case MiniZinc::Expression::E_ITE: {
    auto *ite = MiniZinc::Expression::cast<MiniZinc::ITE>(expr);
    print_ite(ite, indent);
    break;
  }
  case MiniZinc::Expression::E_BINOP:
    fmt::println("E_BINOP");
    break;
  case MiniZinc::Expression::E_UNOP:
    fmt::println("E_UNOP");
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
    fmt::println("E_TI");
    break;
  case MiniZinc::Expression::E_TIID:
    fmt::println("E_TIID");
    break;
  }
}
} // namespace parser
