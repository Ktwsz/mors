#include "ast_printer.hpp"

#include <fmt/base.h>
#include <fmt/format.h>

#include <ranges>

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

void PrintModelVisitor::print_type(MiniZinc::Type const& type,
                                   int const indent) {
  ind(indent);
  fmt::println("Type:");

  ind(indent + 2);
  fmt::println("Decl: {}", type.isPar() ? "parameter" : "variable");

  ind(indent + 2);
  fmt::println("Set: {}", type.isSet());

  ind(indent + 2);
  fmt::println("Base Type:");

  ind(indent + 4);
  switch (type.bt()) {
  case MiniZinc::Type::BaseType::BT_BOOL: {
    fmt::println("BT_BOOL");
    break;
  }
  case MiniZinc::Type::BaseType::BT_INT: {
    fmt::println("BT_INT");
    break;
  }
  case MiniZinc::Type::BaseType::BT_FLOAT: {
    fmt::println("BT_FLOAT");
    break;
  }
  case MiniZinc::Type::BaseType::BT_STRING: {
    fmt::println("BT_STRING");
    break;
  }
  case MiniZinc::Type::BaseType::BT_ANN: {
    fmt::println("BT_ANN");
    break;
  }
  case MiniZinc::Type::BaseType::BT_TUPLE: {
    fmt::println("BT_TUPLE");
    break;
  }
  case MiniZinc::Type::BaseType::BT_RECORD: {
    fmt::println("BT_RECORD");
    break;
  }
  case MiniZinc::Type::BaseType::BT_TOP: {
    fmt::println("BT_TOP");
    break;
  }
  case MiniZinc::Type::BaseType::BT_BOT: {
    fmt::println("BT_BOT");
    break;
  }
  case MiniZinc::Type::BaseType::BT_UNKNOWN: {
    fmt::println("BT_UNKNOWN");
    break;
  }
  }
}

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
    match_expr(var_decl->e(), indent + 4);
  }
}

void PrintModelVisitor::print_type_inst(MiniZinc::TypeInst* type_inst,
                                        int const indent) {
  ind(indent);
  fmt::println("type inst:");

  print_type(type_inst->type(), indent + 2);

  ind(indent + 2);
  fmt::println("Array: {}", type_inst->isarray());

  ind(indent + 2);
  fmt::println("Enum: {}", type_inst->isEnum());

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

void PrintModelVisitor::print_function(MiniZinc::FunctionI* function,
                                       int const indent) {
  ind(indent + 2);
  fmt::println("Return type: ");
  print_type_inst(function->ti(), indent + 4);
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

  ind(indent + 2);
  fmt::println("args:");
  for (auto const& arg : call->args()) {
    match_expr(arg, indent + 4);
  }
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
  ind(indent);
  fmt::println("integer: {}", MiniZinc::IntLit::v(int_lit).toInt());
}

void PrintModelVisitor::print_set_lit(MiniZinc::SetLit* set_lit,
                                      int const indent) {
  ind(indent);
  fmt::println("set:");

  if (auto* isv = set_lit->isv(); isv != nullptr) {
    ind(indent + 2);
    fmt::println("range:");

    ind(indent + 4);
    fmt::println("{}...{}", isv->min().toInt(), isv->max().toInt());
    return;
  }

  for (auto const& expr : set_lit->v()) {
    match_expr(expr, indent + 2);
  }
}

void PrintModelVisitor::print_float_lit(MiniZinc::FloatLit* float_lit,
                                        int const indent) {
  ind(indent);
  fmt::println("float");
  ind(indent + 2);
  fmt::println("value: {}", MiniZinc::FloatLit::v(float_lit).toDouble());
}

void PrintModelVisitor::print_bin_op(MiniZinc::BinOp* bin_op,
                                     int const indent) {
  ind(indent);
  fmt::println("Bin Op");

  // print_type(MiniZinc::Expression::type(bin_op), indent + 2);

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
  case MiniZinc::BOT_NQ: {
    fmt::println("!=");
    break;
  }
  case MiniZinc::BOT_PLUS: {
    fmt::println("BOT_PLUS");
    break;
  }
  case MiniZinc::BOT_MINUS: {
    fmt::println("BOT_MINUS");
    break;
  }
  case MiniZinc::BOT_MULT: {
    fmt::println("BOT_MULT");
    break;
  }
  case MiniZinc::BOT_DIV: {
    fmt::println("BOT_DIV");
    break;
  }
  case MiniZinc::BOT_IDIV: {
    fmt::println("BOT_IDIV");
    break;
  }
  case MiniZinc::BOT_MOD: {
    fmt::println("BOT_MOD");
    break;
  }
  case MiniZinc::BOT_POW: {
    fmt::println("BOT_POW");
    break;
  }
  case MiniZinc::BOT_LE: {
    fmt::println("BOT_LE");
    break;
  }
  case MiniZinc::BOT_LQ: {
    fmt::println("BOT_LQ");
    break;
  }
  case MiniZinc::BOT_GR: {
    fmt::println("BOT_GR");
    break;
  }
  case MiniZinc::BOT_GQ: {
    fmt::println("BOT_GQ");
    break;
  }
  case MiniZinc::BOT_EQ: {
    fmt::println("BOT_EQ");
    break;
  }
  case MiniZinc::BOT_IN: {
    fmt::println("BOT_IN");
    break;
  }
  case MiniZinc::BOT_SUBSET: {
    fmt::println("BOT_SUBSET");
    break;
  }
  case MiniZinc::BOT_SUPERSET: {
    fmt::println("BOT_SUPERSET");
    break;
  }
  case MiniZinc::BOT_UNION: {
    fmt::println("BOT_UNION");
    break;
  }
  case MiniZinc::BOT_DIFF: {
    fmt::println("BOT_DIFF");
    break;
  }
  case MiniZinc::BOT_SYMDIFF: {
    fmt::println("BOT_SYMDIFF");
    break;
  }
  case MiniZinc::BOT_INTERSECT: {
    fmt::println("BOT_INTERSECT");
    break;
  }
  case MiniZinc::BOT_PLUSPLUS: {
    fmt::println("BOT_PLUSPLUS");
    break;
  }
  case MiniZinc::BOT_EQUIV: {
    fmt::println("BOT_EQUIV");
    break;
  }
  case MiniZinc::BOT_IMPL: {
    fmt::println("BOT_IMPL");
    break;
  }
  case MiniZinc::BOT_RIMPL: {
    fmt::println("BOT_RIMPL");
    break;
  }
  case MiniZinc::BOT_OR: {
    fmt::println("BOT_OR");
    break;
  }
  case MiniZinc::BOT_AND: {
    fmt::println("BOT_AND");
    break;
  }
  case MiniZinc::BOT_XOR: {
    fmt::println("BOT_XOR");
    break;
  }
  }

  ind(indent + 2);
  fmt::println("rhs: ");
  match_expr(bin_op->rhs(), indent + 4);
}

void PrintModelVisitor::print_comprehension(MiniZinc::Comprehension* comp,
                                            int const indent) {
  ind(indent);
  fmt::println("comprehension");

  ind(indent + 2);
  fmt::println("generators:");

  for (auto const i :
       std::views::iota(0u, comp->numberOfGenerators()) | std::views::reverse) {

    for (auto const j :
         std::views::iota(0u, comp->numberOfDecls(i)) | std::views::reverse) {
      match_expr(comp->decl(i, j), indent + 4);
    }

    if (comp->in(i) != nullptr) {
      ind(indent + 4);
      fmt::println("in:");
      match_expr(comp->in(i), indent + 6);
    }

    // TODO
    // if (comp->where(i) != nullptr) {
    //   ind(indent + 4);
    //   fmt::println("where:");
    //   match_expr(comp->where(i), indent + 6);
    // }
  }

  ind(indent + 2);
  fmt::println("body:");
  match_expr(comp->e(), indent + 4);
}

void PrintModelVisitor::print_array_lit(MiniZinc::ArrayLit* array_lit,
                                        int const indent) {
  ind(indent);
  fmt::println("ARRAYLIT");

  for (auto const& expr : array_lit->getVec()) {
    match_expr(expr, indent + 2);
  }
}

void PrintModelVisitor::print_array_access(MiniZinc::ArrayAccess* array_access,
                                           int const indent) {
  ind(indent);
  fmt::println("Array access");

  ind(indent + 2);
  fmt::println("array:");
  match_expr(array_access->v(), indent + 4);

  ind(indent + 2);
  fmt::println("indexes:");
  for (auto& ix : array_access->idx()) {
    match_expr(ix, indent + 4);
  }
}

void PrintModelVisitor::print_solve_type(MiniZinc::SolveI* solve_item) {
  switch (solve_item->st()) {
  case MiniZinc::SolveI::SolveType::ST_MAX: {
    fmt::println("MAX");
    match_expr(solve_item->e(), 2);
    break;
  }
  case MiniZinc::SolveI::SolveType::ST_MIN: {
    fmt::println("MIN");
    match_expr(solve_item->e(), 2);
    break;
  }
  case MiniZinc::SolveI::SolveType::ST_SAT: {
    fmt::println("SAT");
    break;
  }
  }
}

void PrintModelVisitor::match_expr(MiniZinc::Expression* expr,
                                   int const indent) {
  switch (MiniZinc::Expression::eid(expr)) {
  case MiniZinc::Expression::E_INTLIT: {
    auto* int_lit = MiniZinc::Expression::cast<MiniZinc::IntLit>(expr);
    print_int_lit(int_lit, indent);
    break;
  }
  case MiniZinc::Expression::E_FLOATLIT: {
    auto* float_lit = MiniZinc::Expression::cast<MiniZinc::FloatLit>(expr);
    print_float_lit(float_lit, indent);
    break;
  }
  case MiniZinc::Expression::E_SETLIT: {
    auto* set_lit = MiniZinc::Expression::cast<MiniZinc::SetLit>(expr);
    print_set_lit(set_lit, indent);
    break;
  }
  case MiniZinc::Expression::E_BOOLLIT: {
    auto* bool_lit = MiniZinc::Expression::cast<MiniZinc::BoolLit>(expr);
    ind(indent);
    fmt::println("bool: {}", bool_lit->v());
    break;
  }
  case MiniZinc::Expression::E_STRINGLIT: {
    auto* string_lit = MiniZinc::Expression::cast<MiniZinc::StringLit>(expr);

    ind(indent);
    fmt::println("string: {}", string_lit->v().c_str() != nullptr
                                   ? string_lit->v().c_str()
                                   : "nullptr");
    break;
  }
  case MiniZinc::Expression::E_ID: {
    auto* id = MiniZinc::Expression::cast<MiniZinc::Id>(expr);
    ind(indent + 2);
    fmt::println("id: {}", id->v().c_str());
    break;
  }
  case MiniZinc::Expression::E_ANON: {
    ind(indent);
    fmt::println("E_ANON");
    break;
  }
  case MiniZinc::Expression::E_ARRAYLIT: {
    auto* array_lit = MiniZinc::Expression::cast<MiniZinc::ArrayLit>(expr);
    print_array_lit(array_lit, indent);

    break;
  }
  case MiniZinc::Expression::E_ARRAYACCESS: {
    auto* array_access =
        MiniZinc::Expression::cast<MiniZinc::ArrayAccess>(expr);
    print_array_access(array_access, indent);

    break;
  }
  case MiniZinc::Expression::E_FIELDACCESS: {
    ind(indent);
    fmt::println("E_FIELDACCESS");
    break;
  }
  case MiniZinc::Expression::E_COMP: {
    auto* comp = MiniZinc::Expression::cast<MiniZinc::Comprehension>(expr);
    print_comprehension(comp, indent);
    break;
  }
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
  case MiniZinc::Expression::E_UNOP: {
    ind(indent);
    fmt::println("E_UNOP");
    break;
  }
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
  case MiniZinc::Expression::E_TI: {
    ind(indent);
    fmt::println("E_TI");
    break;
  }
  case MiniZinc::Expression::E_TIID: {
    ind(indent);
    fmt::println("E_TIID");
    break;
  }
  }
}
} // namespace parser
