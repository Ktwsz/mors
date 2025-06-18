#include "ast_printer.hpp"

namespace parser {
namespace {
void ind(int const indent) {
  for (int i = 0; i < indent; i += 2)
    std::cout << "| ";
}
} // namespace

bool PrintModelVisitor::enterModel(MiniZinc::Model *) { return true; }

bool PrintModelVisitor::enter(MiniZinc::Item * /*m*/) { return true; }

void PrintModelVisitor::vIncludeI(MiniZinc::IncludeI *include) {
  std::cout << "includeI: " << include->f().c_str() << std::endl;
}

void PrintModelVisitor::vVarDeclI(MiniZinc::VarDeclI *varDecl) {
  if (!varDecl->loc().filename().empty() &&
      varDecl->loc().filename().endsWith("cumulative.mzn")) {
    match_expr(varDecl->e());
  }
}

void PrintModelVisitor::vAssignI(MiniZinc::AssignI * /*ai*/) {
  std::cout << "assignI" << std::endl;
}

void PrintModelVisitor::vConstraintI(MiniZinc::ConstraintI *constraint) {
  std::cout << "constraint expr" << std::endl;
  match_expr(constraint->e());
}

void PrintModelVisitor::vSolveI(MiniZinc::SolveI * /*si*/) { std::cout << "solveI" << std::endl; }

void PrintModelVisitor::vOutputI(MiniZinc::OutputI * /*oi*/) {
  std::cout << "output expr" << std::endl;
}

void PrintModelVisitor::vFunctionI(MiniZinc::FunctionI *functionI) {
  /*std::cout << "functionI: id=" << functionI->id().c_str() << std::endl;*/
}

void PrintModelVisitor::print_var_decl(MiniZinc::VarDecl *var_decl, int const indent) {
  ind(indent);
  std::cout << "variable declaration" << std::endl;
  ind(indent + 2);
  std::cout << "id: " << var_decl->id()->v().c_str() << std::endl;
  data.ids.push_back(var_decl->id()->v().c_str());
}

void PrintModelVisitor::print_fn_call(MiniZinc::Call *call, int const indent) {
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

void PrintModelVisitor::print_ite(MiniZinc::ITE *ite, int const indent) {
  for (unsigned int i = 0; i < ite->size(); i++) {
    ind(indent);
    std::cout << "if-condition: ";
    match_expr(ite->ifExpr(i), indent + 2);
    ind(indent);
    std::cout << "if-body: ";
    match_expr(ite->thenExpr(i), indent + 2);
  }
  ind(indent);
  std::cout << "else: " << std::endl;
  match_expr(ite->elseExpr(), indent + 2);
}

void PrintModelVisitor::print_let_expr(MiniZinc::Let *let, int const indent) {
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

void PrintModelVisitor::match_expr(MiniZinc::Expression *expr, int const indent) {
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
  case MiniZinc::Expression::E_ITE: {
    auto *ite = MiniZinc::Expression::cast<MiniZinc::ITE>(expr);
    print_ite(ite, indent);
    break;
  }
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
} // namespace parser
