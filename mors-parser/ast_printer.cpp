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

bool PrintModelVisitor::enterModel(MiniZinc::Model*) { return true; }

bool PrintModelVisitor::enter(MiniZinc::Item* /*m*/) { return true; }

void PrintModelVisitor::vIncludeI(MiniZinc::IncludeI* include) {
  fmt::println("includeI: {}", include->f().c_str());
}

void PrintModelVisitor::vVarDeclI(MiniZinc::VarDeclI* varDecl) {
  // if (varDecl->loc().filename().endsWith(input_model_path)) {
  match_expr(varDecl->e());
  // }
}

void PrintModelVisitor::vAssignI(MiniZinc::AssignI* /*ai*/) {
  fmt::println("assignI");
}

void PrintModelVisitor::vConstraintI(MiniZinc::ConstraintI* constraint) {
  fmt::println("constraint expr");
  match_expr(constraint->e());
}

void PrintModelVisitor::vSolveI(MiniZinc::SolveI* /*si*/) {
  fmt::println("solveI");
}

void PrintModelVisitor::vOutputI(MiniZinc::OutputI* /*oi*/) {
  fmt::println("output expr");
}

void PrintModelVisitor::vFunctionI(MiniZinc::FunctionI* functionI) {}

void PrintModelVisitor::print_var_decl(MiniZinc::VarDecl* var_decl,
                                       int const indent) {
  if (indent == 0 &&
      !var_decl->item()->loc().filename().endsWith(input_model_path))
    return;

  ind(indent);
  fmt::println("variable declaration");

  ind(indent + 2);
  fmt::println("id: {}", var_decl->id()->v().c_str());

  ind(indent + 2);
  fmt::println("toplevel: {}", var_decl->toplevel());

  ind(indent + 2);
  fmt::println("introduced: {}", var_decl->introduced());

  print_type_inst(var_decl->ti(), indent + 2);

  ind(indent + 2);
  if (var_decl->item()->loc().filename().empty())
    fmt::println("loc: empty");
  else
    fmt::println("loc: {}", var_decl->item()->loc().filename().c_str());

  ind(indent + 2);
  if (var_decl->e() == nullptr) {
    fmt::println("expr: null");
  } else {
    fmt::println("expr: ");
    match_expr(var_decl->e(), indent + 2);
  }
}

void PrintModelVisitor::print_type_inst(MiniZinc::TypeInst* type_inst,
                                        int const indent) {
  ind(indent);
  fmt::println("type:");

  ind(indent + 2);
  fmt::println("ranges:");

  for (auto const& r : type_inst->ranges()) {
    print_type_inst(r, indent + 4);
  }

  ind(indent + 2);
  if (type_inst->domain() == nullptr) {
    fmt::println("domain: null");
  } else {
    fmt::println("domain: ");
    match_expr(type_inst->domain(), indent + 4);
  }
}

void PrintModelVisitor::print_fn_call(MiniZinc::Call* call, int const indent) {
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

void PrintModelVisitor::print_ite(MiniZinc::ITE* ite, int const indent) {
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

void PrintModelVisitor::print_let_expr(MiniZinc::Let* let, int const indent) {
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

void PrintModelVisitor::print_int_lit(MiniZinc::IntLit* int_lit,
                                      int const indent) {
  ind(indent + 2);
  fmt::println("integer");
  ind(indent + 2);
  fmt::println("value: {}", MiniZinc::IntLit::v(int_lit).toInt());
}

void PrintModelVisitor::print_bin_op(MiniZinc::BinOp* bin_op,
                                     int const indent) {
  ind(indent);
  fmt::println("Bin Op");

  ind(indent + 2);
  fmt::println("lhs: ");
  match_expr(bin_op->lhs(), indent + 4);

  ind(indent + 2);
  fmt::print("op: ");
  switch (bin_op->op()) {
  case MiniZinc::BOT_DOTDOT: {
    fmt::println("..");
    break;
  }
  default:
    fmt::println("other");
  }

  ind(indent + 2);
  fmt::println("rhs: ");
  match_expr(bin_op->rhs(), indent + 4);
}

void PrintModelVisitor::match_expr(MiniZinc::Expression* expr,
                                   int const indent) {
  switch (MiniZinc::Expression::eid(expr)) {
  case MiniZinc::Expression::E_INTLIT: {
    auto* int_lit = MiniZinc::Expression::cast<MiniZinc::IntLit>(expr);
    print_int_lit(int_lit, indent);
    break;
  }
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
    auto* ite = MiniZinc::Expression::cast<MiniZinc::ITE>(expr);
    print_ite(ite, indent);
    break;
  }
  case MiniZinc::Expression::E_BINOP: {
    auto* bin_op = MiniZinc::Expression::cast<MiniZinc::BinOp>(expr);
    print_bin_op(bin_op, indent);
    break;
  }
  case MiniZinc::Expression::E_UNOP:
    fmt::println("E_UNOP");
    break;
  case MiniZinc::Expression::E_CALL: {
    auto* call = MiniZinc::Expression::cast<MiniZinc::Call>(expr);
    print_fn_call(call, indent);
    break;
  }
  case MiniZinc::Expression::E_VARDECL: {
    auto* varDecl = MiniZinc::Expression::cast<MiniZinc::VarDecl>(expr);
    print_var_decl(varDecl, indent);
    break;
  }
  case MiniZinc::Expression::E_LET: {
    auto* let = MiniZinc::Expression::cast<MiniZinc::Let>(expr);
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
