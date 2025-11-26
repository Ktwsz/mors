#include "ast_printer.hpp"

#include <ranges>
#include <print>

namespace parser {
namespace {
void ind(int const indent) {
  for (int i = 0; i < indent; i += 2)
    std::print("| ");
}
} // namespace

void PrintModelVisitor::print_type(MiniZinc::Type const& type,
                                   int const indent) {
  ind(indent);
  std::println("Type:");

  ind(indent + 2);
  std::println("Decl: {}", type.isPar() ? "parameter" : "variable");

  ind(indent + 2);
  std::println("Set: {}", type.isSet());

  ind(indent + 2);
  std::println("Base Type:");

  ind(indent + 4);
  switch (type.bt()) {
  case MiniZinc::Type::BaseType::BT_BOOL: {
    std::println("BT_BOOL");
    break;
  }
  case MiniZinc::Type::BaseType::BT_INT: {
    std::println("BT_INT");
    break;
  }
  case MiniZinc::Type::BaseType::BT_FLOAT: {
    std::println("BT_FLOAT");
    break;
  }
  case MiniZinc::Type::BaseType::BT_STRING: {
    std::println("BT_STRING");
    break;
  }
  case MiniZinc::Type::BaseType::BT_ANN: {
    std::println("BT_ANN");
    break;
  }
  case MiniZinc::Type::BaseType::BT_TUPLE: {
    std::println("BT_TUPLE");
    break;
  }
  case MiniZinc::Type::BaseType::BT_RECORD: {
    std::println("BT_RECORD");
    break;
  }
  case MiniZinc::Type::BaseType::BT_TOP: {
    std::println("BT_TOP");
    break;
  }
  case MiniZinc::Type::BaseType::BT_BOT: {
    std::println("BT_BOT");
    break;
  }
  case MiniZinc::Type::BaseType::BT_UNKNOWN: {
    std::println("BT_UNKNOWN");
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
  std::println("variable declaration");

  ind(indent + 2);
  std::println("id: {}", var_decl->id()->v().c_str());

  ind(indent + 2);
  std::println("toplevel: {}", var_decl->toplevel());

  ind(indent + 2);
  std::println("introduced: {}", var_decl->introduced());

  print_type_inst(var_decl->ti(), indent + 2);

  ind(indent + 2);
  if (var_decl->item()->loc().filename().empty())
    std::println("loc: empty");
  else
    std::println("loc: {}", var_decl->item()->loc().filename().c_str());

  ind(indent + 2);
  if (var_decl->e() == nullptr) {
    std::println("expr: null");
  } else {
    std::println("expr: ");
    match_expr(var_decl->e(), indent + 4);
  }
}

void PrintModelVisitor::print_type_inst(MiniZinc::TypeInst* type_inst,
                                        int const indent) {
  ind(indent);
  std::println("type inst:");

  print_type(type_inst->type(), indent + 2);

  ind(indent + 2);
  std::println("Array: {}", type_inst->isarray());

  ind(indent + 2);
  std::println("Enum: {}", type_inst->isEnum());

  ind(indent + 2);
  std::println("ranges:");

  for (auto const& r : type_inst->ranges()) {
    print_type_inst(r, indent + 4);
  }

  ind(indent + 2);
  if (type_inst->domain() == nullptr) {
    std::println("domain: null");
  } else {
    std::println("domain: ");
    match_expr(type_inst->domain(), indent + 4);
  }
}

void PrintModelVisitor::print_fn_call(MiniZinc::Call* call, int const indent) {
  auto const functionItem = model.matchFn(env, call, true, false);

  ind(indent);
  std::println("function call");

  ind(indent + 2);
  std::println("is_bool: {}", MiniZinc::Expression::type(call).isbool());

  ind(indent + 2);
  std::println("id: {}", functionItem->id().c_str());

  if (functionItem->e()) {
    ind(indent + 2);
    std::println("return:");
    print_type_inst(functionItem->ti(), indent + 4);

    ind(indent + 2);
    std::println("function params: ");
    for (auto const ix : std::views::iota(0u, functionItem->paramCount())) {
      print_var_decl(functionItem->param(ix), indent + 4);
      functionItem->param(ix)->e(call->arg(ix));
    }

    ind(indent + 2);
    std::println("function body: ");

    match_expr(functionItem->e(), indent + 4);

    for (auto const ix : std::views::iota(0u, functionItem->paramCount()))
      functionItem->param(ix)->e(nullptr);

    ind(indent + 2);
    std::println("args:");
    for (auto const& arg : call->args()) {
      match_expr(arg, indent + 4);
    }

  } else if (functionItem->builtins.e != nullptr) {
    ind(indent + 2);
    std::println("builtin: e");

    ind(indent + 2);
    std::println("result:");
    match_expr(functionItem->builtins.e(env, call), indent + 4);

  } else if (functionItem->builtins.b != nullptr) {

    ind(indent + 2);
    std::println("builtin: b");

    ind(indent + 2);
    std::println("result: {}", functionItem->builtins.b(env, call));

  } else if (functionItem->builtins.f != nullptr) {

    ind(indent + 2);
    std::println("builtin: f");

    ind(indent + 2);
    std::println("result: {}", functionItem->builtins.f(env, call).toDouble());

  } else if (functionItem->builtins.fs != nullptr) {

    ind(indent + 2);
    std::println("builtin: fs");

  } else if (functionItem->builtins.i != nullptr) {

    ind(indent + 2);
    std::println("builtin: i");

    ind(indent + 2);
    std::println("result: {}", functionItem->builtins.i(env, call).toInt());

  } else if (functionItem->builtins.s != nullptr) {

    ind(indent + 2);
    std::println("builtin: s");

  } else if (functionItem->builtins.str != nullptr) {

    ind(indent + 2);
    std::println("builtin: str");
  }
}

void PrintModelVisitor::print_ite(MiniZinc::ITE* ite, int const indent) {
  for (unsigned int i = 0; i < ite->size(); i++) {
    ind(indent);
    std::println("if-condition: ");
    match_expr(ite->ifExpr(i), indent + 2);

    ind(indent);
    std::println("if-body: ");
    match_expr(ite->thenExpr(i), indent + 2);
  }

  ind(indent);
  std::println("else: ");
  match_expr(ite->elseExpr(), indent + 2);
}

void PrintModelVisitor::print_let_expr(MiniZinc::Let* let, int const indent) {
  ind(indent);
  std::println("let: ");

  ind(indent + 2);
  std::println("declarations");

  for (auto e : let->let()) {
    match_expr(e, indent + 4);
  }

  ind(indent);
  std::println("in: ");

  match_expr(let->in(), indent + 2);
  std::print("\n");
}

void PrintModelVisitor::print_int_lit(MiniZinc::IntLit* int_lit,
                                      int const indent) {
  ind(indent);
  std::println("integer: {}", MiniZinc::IntLit::v(int_lit).toInt());
}

void PrintModelVisitor::print_set_lit(MiniZinc::SetLit* set_lit,
                                      int const indent) {
  ind(indent);
  std::println("set:");

  if (auto* isv = set_lit->isv(); isv != nullptr) {
    ind(indent + 2);
    std::println("range:");

    ind(indent + 4);
    std::println("{}...{}", isv->min().toInt(), isv->max().toInt());
    return;
  }

  for (auto const& expr : set_lit->v()) {
    match_expr(expr, indent + 2);
  }
}

void PrintModelVisitor::print_float_lit(MiniZinc::FloatLit* float_lit,
                                        int const indent) {
  ind(indent);
  std::println("float");
  ind(indent + 2);
  std::println("value: {}", MiniZinc::FloatLit::v(float_lit).toDouble());
}

void PrintModelVisitor::print_bin_op(MiniZinc::BinOp* bin_op,
                                     int const indent) {
  ind(indent);
  std::println("Bin Op");

  // print_type(MiniZinc::Expression::type(bin_op), indent + 2);

  ind(indent + 2);
  std::println("lhs: ");
  match_expr(bin_op->lhs(), indent + 4);

  ind(indent + 2);
  std::print("op: ");
  switch (bin_op->op()) {
  case MiniZinc::BOT_DOTDOT: {
    std::println("..");
    break;
  }
  case MiniZinc::BOT_NQ: {
    std::println("!=");
    break;
  }
  case MiniZinc::BOT_PLUS: {
    std::println("BOT_PLUS");
    break;
  }
  case MiniZinc::BOT_MINUS: {
    std::println("BOT_MINUS");
    break;
  }
  case MiniZinc::BOT_MULT: {
    std::println("BOT_MULT");
    break;
  }
  case MiniZinc::BOT_DIV: {
    std::println("BOT_DIV");
    break;
  }
  case MiniZinc::BOT_IDIV: {
    std::println("BOT_IDIV");
    break;
  }
  case MiniZinc::BOT_MOD: {
    std::println("BOT_MOD");
    break;
  }
  case MiniZinc::BOT_POW: {
    std::println("BOT_POW");
    break;
  }
  case MiniZinc::BOT_LE: {
    std::println("BOT_LE");
    break;
  }
  case MiniZinc::BOT_LQ: {
    std::println("BOT_LQ");
    break;
  }
  case MiniZinc::BOT_GR: {
    std::println("BOT_GR");
    break;
  }
  case MiniZinc::BOT_GQ: {
    std::println("BOT_GQ");
    break;
  }
  case MiniZinc::BOT_EQ: {
    std::println("BOT_EQ");
    break;
  }
  case MiniZinc::BOT_IN: {
    std::println("BOT_IN");
    break;
  }
  case MiniZinc::BOT_SUBSET: {
    std::println("BOT_SUBSET");
    break;
  }
  case MiniZinc::BOT_SUPERSET: {
    std::println("BOT_SUPERSET");
    break;
  }
  case MiniZinc::BOT_UNION: {
    std::println("BOT_UNION");
    break;
  }
  case MiniZinc::BOT_DIFF: {
    std::println("BOT_DIFF");
    break;
  }
  case MiniZinc::BOT_SYMDIFF: {
    std::println("BOT_SYMDIFF");
    break;
  }
  case MiniZinc::BOT_INTERSECT: {
    std::println("BOT_INTERSECT");
    break;
  }
  case MiniZinc::BOT_PLUSPLUS: {
    std::println("BOT_PLUSPLUS");
    break;
  }
  case MiniZinc::BOT_EQUIV: {
    std::println("BOT_EQUIV");
    break;
  }
  case MiniZinc::BOT_IMPL: {
    std::println("BOT_IMPL");
    break;
  }
  case MiniZinc::BOT_RIMPL: {
    std::println("BOT_RIMPL");
    break;
  }
  case MiniZinc::BOT_OR: {
    std::println("BOT_OR");
    break;
  }
  case MiniZinc::BOT_AND: {
    std::println("BOT_AND");
    break;
  }
  case MiniZinc::BOT_XOR: {
    std::println("BOT_XOR");
    break;
  }
  }

  ind(indent + 2);
  std::println("rhs: ");
  match_expr(bin_op->rhs(), indent + 4);
}

void PrintModelVisitor::print_un_op(MiniZinc::UnOp* un_op, int const indent) {
  ind(indent);
  std::println("Unary Op");

  ind(indent + 2);
  std::print("op: ");
  switch (un_op->op()) {
  case MiniZinc::UOT_NOT: {
    std::println("UOT_NOT");
    break;
  }
  case MiniZinc::UOT_PLUS: {
    std::println("UOT_PLUS");
    break;
  }
  case MiniZinc::UOT_MINUS: {
    std::println("UOT_MINUS");
    break;
  }
  }

  ind(indent + 2);
  std::println("expr: ");
  match_expr(un_op->e(), indent + 4);
}

void PrintModelVisitor::print_comprehension(MiniZinc::Comprehension* comp,
                                            int const indent) {
  ind(indent);
  std::println("comprehension");

  ind(indent + 2);
  std::println("generators:");

  for (auto const i : std::views::iota(0u, comp->numberOfGenerators())) {

    for (auto const j : std::views::iota(0u, comp->numberOfDecls(i))) {
      match_expr(comp->decl(i, j), indent + 4);
    }

    if (comp->in(i) != nullptr) {
      ind(indent + 4);
      std::println("in:");
      match_expr(comp->in(i), indent + 6);
    }

    if (comp->where(i) != nullptr) {
      ind(indent + 4);
      std::println("where:");
      match_expr(comp->where(i), indent + 6);
    }
  }

  ind(indent + 2);
  std::println("body:");
  match_expr(comp->e(), indent + 4);
}

void PrintModelVisitor::print_array_lit(MiniZinc::ArrayLit* array_lit,
                                        int const indent) {
  ind(indent);
  std::println("ARRAYLIT");

  for (auto const& expr : array_lit->getVec()) {
    match_expr(expr, indent + 2);
  }
}

void PrintModelVisitor::print_array_access(MiniZinc::ArrayAccess* array_access,
                                           int const indent) {
  ind(indent);
  std::println("Array access");

  ind(indent + 2);
  std::println("array:");
  match_expr(array_access->v(), indent + 4);

  ind(indent + 2);
  std::println("indexes:");
  for (auto& ix : array_access->idx()) {
    match_expr(ix, indent + 4);
  }
}

void PrintModelVisitor::print_solve_type(MiniZinc::SolveI* solve_item) {
  switch (solve_item->st()) {
  case MiniZinc::SolveI::SolveType::ST_MAX: {
    std::println("MAX");
    match_expr(solve_item->e(), 2);
    break;
  }
  case MiniZinc::SolveI::SolveType::ST_MIN: {
    std::println("MIN");
    match_expr(solve_item->e(), 2);
    break;
  }
  case MiniZinc::SolveI::SolveType::ST_SAT: {
    std::println("SAT");
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
    std::println("bool: {}", bool_lit->v());
    break;
  }
  case MiniZinc::Expression::E_STRINGLIT: {
    auto* string_lit = MiniZinc::Expression::cast<MiniZinc::StringLit>(expr);

    ind(indent);
    std::println("string: {}", string_lit->v().c_str() != nullptr
                                   ? string_lit->v().c_str()
                                   : "nullptr");
    break;
  }
  case MiniZinc::Expression::E_ID: {
    auto* id = MiniZinc::Expression::cast<MiniZinc::Id>(expr);
    ind(indent);
    std::println("id: {}", id->v().c_str());
    break;
  }
  case MiniZinc::Expression::E_ANON: {
    ind(indent);
    std::println("E_ANON");
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
    std::println("E_FIELDACCESS");
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
    auto* un_op = MiniZinc::Expression::cast<MiniZinc::UnOp>(expr);
    print_un_op(un_op, indent);
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
    std::println("E_TI");
    break;
  }
  case MiniZinc::Expression::E_TIID: {
    ind(indent);
    std::println("E_TIID");
    break;
  }
  }
}
} // namespace parser
