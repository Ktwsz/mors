#include "transformer.hpp"
#include "ast.hpp"

#include <fmt/base.h>
#include <minizinc/type.hh>

#include <optional>
#include <ranges>
#include <string_view>
#include <utility>
#include <variant>

namespace parser {
namespace {
template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};

template <typename T>
concept HasType = requires(T t) {
  { t.expr_type } -> std::same_as<ast::Type&>;
};

auto resolve_base_type(MiniZinc::Type::BaseType base_type) -> ast::Type {
  // enum BaseType {
  //   BT_ANN,
  //   BT_TUPLE,
  //   BT_RECORD,
  //   BT_TOP,
  //   BT_BOT,
  //   BT_UNKNOWN
  // };
  switch (base_type) {
  case MiniZinc::Type::BT_INT: {
    return ast::types::Int{};
  }
  case MiniZinc::Type::BT_FLOAT: {
    return ast::types::Float{};
  }
  case MiniZinc::Type::BT_BOOL: {
    return ast::types::Bool{};
  }
  case MiniZinc::Type::BT_STRING: {
    return ast::types::String{};
  }
  case MiniZinc::Type::BT_TOP: {
    return ast::types::Int{}; // TODO?
  }
  default:
    assert(false);
  }
}

auto reformat_id(std::string_view const id) -> std::string {
  if (id[0] != '\\') {
    return std::string{id};
  }

  size_t const ix = id.find('@');

  return std::string{id.substr(ix + 1)} + "_" +
         std::string{id.substr(1, ix - 1)};
}
} // namespace

auto Transformer::map(MiniZinc::TypeInst* type_inst) -> ast::Type {
  if (type_inst->isarray()) {
    ast::types::Array arr{};

    for (auto inner : type_inst->ranges()) {
      if (inner->domain())
        arr.dims.push_back(*map(inner->domain()));
      else
        arr.dims.push_back(std::nullopt);
    }

    return arr;
  }

  auto const& type = type_inst->type();
  if (type.isSet())
    return std::visit(overloaded{[](ast::types::Int const&) -> ast::Type {
                                   return ast::types::IntSet{};
                                 },
                                 [](ast::types::Float const&) -> ast::Type {
                                   return ast::types::FloatSet{};
                                 },
                                 [](ast::types::Bool const&) -> ast::Type {
                                   return ast::types::BoolSet{};
                                 },
                                 [](auto const& t) -> ast::Type { return t; }},
                      resolve_base_type(type.bt()));

  return resolve_base_type(type.bt());
}

auto Transformer::handle_const_decl(MiniZinc::VarDecl* var_decl)
    -> ast::VarDecl {
  return ast::DeclConst{.id = std::string{var_decl->id()->v().c_str()},
                        .type = map(var_decl->ti()),
                        .value =
                            var_decl->e() ? map(var_decl->e()) : std::nullopt};
}

auto Transformer::handle_var_decl(MiniZinc::VarDecl* var_decl) -> ast::VarDecl {
  return ast::DeclVariable{.id = std::string{var_decl->id()->v().c_str()},
                           .var_type = map(var_decl->ti()),
                           .domain = var_decl->ti()->domain() != nullptr
                                       ? map(var_decl->ti()->domain())
                                       : std::nullopt};
}

auto Transformer::map(MiniZinc::VarDecl* var_decl, bool const is_global)
    -> std::optional<ast::VarDecl> {
  if (!var_decl->item()->loc().filename().endsWith(input_model_path) ||
      var_decl->id()->str() == "_objective")
    return std::nullopt;

  auto var = MiniZinc::Expression::type(var_decl->ti()).isPar()
               ? handle_const_decl(var_decl)
               : handle_var_decl(var_decl);

  std::visit(overloaded{
                 [&](ast::DeclVariable& var) { var.is_global = is_global; },
                 [&](ast::DeclConst& var) { var.is_global = is_global; },
             },
             var);
  variable_map[std::visit(
                   overloaded{
                       [](ast::DeclVariable const& var) { return var.id; },
                       [](ast::DeclConst const& var) { return var.id; },
                   },
                   var)]
      .push_back(var);

  return var;
}

auto Transformer::map(MiniZinc::SetLit* set_lit) -> ast::ExprHandle {
  if (auto* isv = set_lit->isv(); isv != nullptr) {
    auto const& min = isv->min();
    auto const& max = isv->max();

    assert(min.isFinite());
    assert(max.isFinite());

    return std::make_shared<ast::Expr>(ast::BinOp{
        .kind = ast::BinOp::OpKind::DOTDOT,
        .lhs = std::make_shared<ast::Expr>(ast::LiteralInt{min.toInt()}),
        .rhs = std::make_shared<ast::Expr>(ast::LiteralInt{max.toInt()}),
        .expr_type = ast::types::IntSet{}});
  }

  auto ast_set = std::make_shared<ast::Expr>(ast::LiteralSet{});

  auto& ast_set_ref = std::get<ast::LiteralArray>(*ast_set);
  for (auto& set_expr : set_lit->v()) {
    auto ast_expr = map(set_expr);
    assert(ast_expr && "Set Literal: nullopt item");

    ast_set_ref.value.push_back(*ast_expr);
  }

  return ast_set;
}

auto Transformer::map(MiniZinc::Comprehension* comp) -> ast::Comprehension {
  std::vector<ast::Generator> generators;
  std::vector<std::string> decls_to_pop;

  for (auto const i : std::views::iota(0u, comp->numberOfGenerators())) {

    assert((comp->in(i) != nullptr || comp->where(i) != nullptr) &&
           "Null generator");

    if (comp->in(i) != nullptr) {
      auto const in_expr = map(comp->in(i));

      for (auto const j : std::views::iota(0u, comp->numberOfDecls(i))) {
        auto decl_expr = handle_const_decl(comp->decl(i, j));
        assert(std::holds_alternative<ast::DeclConst>(decl_expr));

        std::string const& tmp_id = std::get<ast::DeclConst>(decl_expr).id;
        decls_to_pop.push_back(tmp_id);
        variable_map[tmp_id].push_back(decl_expr);

        generators.push_back(ast::Iterator{
            .variable = std::get<ast::DeclConst>(decl_expr), .in = *in_expr});
      }
    }

    if (comp->where(i) != nullptr) {
      auto const where_expr = map(comp->where(i));
      assert(where_expr && "Null filter after mapping");
      generators.push_back(*where_expr);
    }
  }

  auto body = map(comp->e());
  assert(body);

  for (auto const& s : decls_to_pop)
    variable_map[s].pop_back();

  return ast::Comprehension{.body = std::move(*body),
                            .generators = std::move(generators)};
}

void Transformer::save(MiniZinc::FunctionI* function) {
  if (function->e() == nullptr)
    return;

  std::vector<std::string> decls_to_pop;

  auto const id = reformat_id(std::string{function->id().c_str()});

  std::vector<std::string> params;
  for (auto const ix : std::views::iota(0u, function->paramCount())) {
    auto const& id = function->param(ix)->id();
    assert(id->v().c_str() != nullptr && id->v() != "");
    params.push_back(std::string{id->v().c_str()});

    auto decl_expr = handle_const_decl(function->param(ix));
    std::string tmp_id =
        std::visit(overloaded{[](ast::DeclVariable const& v) { return v.id; },
                              [](ast::DeclConst const& v) { return v.id; }},
                   decl_expr);
    decls_to_pop.push_back(tmp_id);
    variable_map[tmp_id].push_back(decl_expr);
  }

  auto const function_body = map(function->e());
  assert(function_body);

  for (auto const& s : decls_to_pop)
    variable_map[s].pop_back();

  functions.emplace(id, ast::Function{id, params, *function_body});
}

auto Transformer::map(MiniZinc::Expression* expr)
    -> std::optional<ast::ExprHandle> {
  switch (MiniZinc::Expression::eid(expr)) {
  case MiniZinc::Expression::E_INTLIT: {
    auto const* int_lit = MiniZinc::Expression::cast<MiniZinc::IntLit>(expr);
    auto const value = MiniZinc::IntLit::v(int_lit);

    assert(value.isFinite());

    return std::make_shared<ast::Expr>(ast::LiteralInt{value.toInt()});
  }
  case MiniZinc::Expression::E_ID: {
    auto* id = MiniZinc::Expression::cast<MiniZinc::Id>(expr);
    assert(id->v().c_str() != nullptr && id->v() != "" &&
           "Id Expression: null id");

    std::string id_str{id->v().c_str()};
    auto var = variable_map[id_str].back();

    return std::make_shared<ast::Expr>(ast::IdExpr{
        .id = id_str,
        .is_global = std::visit(
            overloaded{
                [](ast::DeclVariable const& var) { return var.is_global; },
                [](ast::DeclConst const& var) { return var.is_global; },
            },
            var),
        .is_var = std::visit(overloaded{
                                 [](ast::DeclVariable const&) { return true; },
                                 [](ast::DeclConst const&) { return false; },
                             },
                             var),
        .expr_type = std::visit(
            overloaded{
                [](ast::DeclVariable const& var) { return var.var_type; },
                [](ast::DeclConst const& var) { return var.type; },
            },
            var)});
  }
  case MiniZinc::Expression::E_BINOP: {
    auto* bin_op = MiniZinc::Expression::cast<MiniZinc::BinOp>(expr);
    return map(bin_op);
  }
  case MiniZinc::Expression::E_UNOP: {
    auto* un_op = MiniZinc::Expression::cast<MiniZinc::UnOp>(expr);
    return map(un_op);
  }
  case MiniZinc::Expression::E_CALL: {
    auto* call = MiniZinc::Expression::cast<MiniZinc::Call>(expr);

    assert(call->id().c_str() != nullptr && "Function call: null function id");
    auto const id = reformat_id(std::string{call->id().c_str()});
    assert(!id.empty() && "Function call: empty function id");

    if (id == "assert") {
      fmt::println("skipping assert calls for now");
      return std::nullopt;
    }

    auto const function_item = model.matchFn(env, call, true, false);
    save(function_item);

    auto ast_call = std::make_shared<ast::Expr>(
        ast::Call{.id = id, .args = {}, .expr_type = map(function_item->ti())});

    for (auto& call_arg : call->args()) {
      auto ast_arg = map(call_arg);
      assert(ast_arg && "Function call: nullopt arg");

      std::get<ast::Call>(*ast_call).args.push_back(*ast_arg);
    }

    return ast_call;
  }
  case MiniZinc::Expression::E_FLOATLIT: {
    auto const* float_lit =
        MiniZinc::Expression::cast<MiniZinc::FloatLit>(expr);
    auto const value = MiniZinc::FloatLit::v(float_lit);

    assert(value.isFinite());

    return std::make_shared<ast::Expr>(ast::LiteralFloat{value.toDouble()});
  }
  case MiniZinc::Expression::E_SETLIT: {
    auto* set_lit = MiniZinc::Expression::cast<MiniZinc::SetLit>(expr);
    return map(set_lit);
  }
  case MiniZinc::Expression::E_BOOLLIT: {
    auto* bool_lit = MiniZinc::Expression::cast<MiniZinc::BoolLit>(expr);
    return std::make_shared<ast::Expr>(ast::LiteralBool{bool_lit->v()});
  }
  case MiniZinc::Expression::E_STRINGLIT: {
    auto* string_lit = MiniZinc::Expression::cast<MiniZinc::StringLit>(expr);
    return std::make_shared<ast::Expr>(
        ast::LiteralString{string_lit->v().c_str() != nullptr
                               ? std::string{string_lit->v().c_str()}
                               : std::string{}});
  }
  case MiniZinc::Expression::E_ARRAYLIT: {
    auto* array_lit = MiniZinc::Expression::cast<MiniZinc::ArrayLit>(expr);
    auto ast_array = std::make_shared<ast::Expr>(ast::LiteralArray{});
    for (auto& array_expr : array_lit->getVec()) {
      auto ast_expr = map(array_expr);
      assert(ast_expr && "Array Literal: nullopt item");

      std::get<ast::LiteralArray>(*ast_array).value.push_back(*ast_expr);
    }

    return ast_array;
  }
  case MiniZinc::Expression::E_COMP: {
    auto* comp = MiniZinc::Expression::cast<MiniZinc::Comprehension>(expr);
    return std::make_shared<ast::Expr>(map(comp));
  }
  case MiniZinc::Expression::E_ARRAYACCESS: {
    auto* array_access =
        MiniZinc::Expression::cast<MiniZinc::ArrayAccess>(expr);

    auto ast_array = std::make_shared<ast::Expr>(ast::ArrayAccess{});

    auto arr_expr = map(array_access->v());
    assert(arr_expr && "Array Access: nullopt array");
    std::get<ast::ArrayAccess>(*ast_array).arr = *arr_expr;

    for (auto& ix : array_access->idx()) {
      auto ix_expr = map(ix);
      assert(ix_expr && "Array Access: nullopt index");

      std::get<ast::ArrayAccess>(*ast_array).indexes.push_back(*ix_expr);
    }

    return ast_array;
  }
  case MiniZinc::Expression::E_ITE: {
    auto* ite = MiniZinc::Expression::cast<MiniZinc::ITE>(expr);
    return map(ite);
  }
  case MiniZinc::Expression::E_VARDECL: {
    assert(false && "Should not reach var decl through this function");
  }
  // case MiniZinc::Expression::E_ANON:
  //   fmt::println("E_ANON");
  //   break;
  // case MiniZinc::Expression::E_FIELDACCESS:
  //   fmt::println("E_FIELDACCESS");
  //   break;
  // case MiniZinc::Expression::E_LET: {
  //   auto* let = MiniZinc::Expression::cast<MiniZinc::Let>(expr);
  //   print_let_expr(let, indent);
  //   break;
  // }
  // case MiniZinc::Expression::E_TI:
  //   fmt::println("E_TI");
  //   break;
  // case MiniZinc::Expression::E_TIID:
  //   fmt::println("E_TIID");
  //   break;
  // }
  default:
    assert(false);
  }
}

auto Transformer::map(MiniZinc::BinOp* bin_op) -> ast::ExprHandle {
  auto match_kind = [](MiniZinc::BinOpType const t) {
    switch (t) {
    case MiniZinc::BOT_DOTDOT:
      return ast::BinOp::OpKind::DOTDOT;
    case MiniZinc::BOT_NQ:
      return ast::BinOp::OpKind::NQ;
    case MiniZinc::BOT_PLUSPLUS:
      return ast::BinOp::OpKind::PLUSPLUS;
    case MiniZinc::BOT_PLUS:
      return ast::BinOp::OpKind::PLUS;
    case MiniZinc::BOT_MINUS:
      return ast::BinOp::OpKind::MINUS;
    case MiniZinc::BOT_MULT:
      return ast::BinOp::OpKind::MULT;
    case MiniZinc::BOT_IDIV:
      return ast::BinOp::OpKind::IDIV;
    case MiniZinc::BOT_MOD:
      return ast::BinOp::OpKind::MOD;
    case MiniZinc::BOT_EQ:
      return ast::BinOp::OpKind::EQ;
    case MiniZinc::BOT_GQ:
      return ast::BinOp::OpKind::GQ;
    case MiniZinc::BOT_GR:
      return ast::BinOp::OpKind::GR;
    case MiniZinc::BOT_LE:
      return ast::BinOp::OpKind::LE;
    case MiniZinc::BOT_LQ:
      return ast::BinOp::OpKind::LQ;
    case MiniZinc::BOT_AND:
      return ast::BinOp::OpKind::AND;
    case MiniZinc::BOT_OR:
      return ast::BinOp::OpKind::OR;
    default:
      assert(false);
    }
  };

  auto lhs = map(bin_op->lhs());
  auto rhs = map(bin_op->rhs());
  assert(lhs && rhs);

  auto expr_type = std::visit(
      overloaded{
          [](HasType auto const& t) { return t.expr_type; },
      },
      **lhs);

  return std::make_shared<ast::Expr>(
      ast::BinOp{.kind = match_kind(bin_op->op()),
                 .lhs = std::move(*lhs),
                 .rhs = std::move(*rhs),
                 .expr_type = std::move(expr_type)});
}

auto Transformer::map(MiniZinc::UnOp* un_op) -> ast::ExprHandle {
  auto match_kind = [](MiniZinc::UnOpType const t) {
    switch (t) {
    case MiniZinc::UOT_NOT:
      return ast::UnaryOp::OpKind::NOT;
    case MiniZinc::UOT_PLUS:
      return ast::UnaryOp::OpKind::PLUS;
    case MiniZinc::UOT_MINUS:
      return ast::UnaryOp::OpKind::MINUS;
    }
  };

  auto expr = map(un_op->e());
  assert(expr);

  auto expr_type = std::visit(
      overloaded{
          [](HasType auto const& t) { return t.expr_type; },
      },
      **expr);

  return std::make_shared<ast::Expr>(
      ast::UnaryOp{.kind = match_kind(un_op->op()),
                 .expr = std::move(*expr),
                 .expr_type = std::move(expr_type)});
}

auto Transformer::map(MiniZinc::ITE* ite) -> ast::ExprHandle {
  auto result = std::make_shared<ast::Expr>(ast::IfThenElse{});
  auto& ite_result = std::get<ast::IfThenElse>(*result);

  for (unsigned int i : std::views::iota(0u, ite->size())) {
    auto const if_expr = map(ite->ifExpr(i));
    auto const then_expr = map(ite->thenExpr(i));

    assert(if_expr && then_expr && "If or then expr is null");

    ite_result.if_then.push_back({std::move(*if_expr), std::move(*then_expr)});
  }

  if (auto* else_expr = ite->elseExpr(); else_expr != nullptr)
    ite_result.else_expr = map(else_expr);

  ite_result.expr_type = std::visit(
      overloaded{
          [](HasType auto const& t) { return t.expr_type; },
      },
      *ite_result.if_then.front().second);

  return result;
}

auto Transformer::map(MiniZinc::SolveI* solve_item) -> ast::SolveType {
  switch (solve_item->st()) {
  case MiniZinc::SolveI::SolveType::ST_MAX:
    return ast::solve_type::Max{.expr = find_objective_expr()};
  case MiniZinc::SolveI::SolveType::ST_MIN:
    return ast::solve_type::Min{.expr = find_objective_expr()};
  case MiniZinc::SolveI::SolveType::ST_SAT:
    return ast::solve_type::Sat{};
  }
  assert(false);
}

auto Transformer::find_objective_expr() -> ast::ExprHandle {
  for (auto& var_decl_item : model.vardecls()) { // TODO: add const version
    auto var_decl =
        MiniZinc::Expression::cast<MiniZinc::VarDecl>(var_decl_item.e());
    if (var_decl->id()->str() != "_objective")
      continue;

    assert(var_decl->e());
    auto var_decl_expr = map(var_decl->e());
    assert(var_decl_expr);

    return *var_decl_expr;
  }

  assert(false);
}

} // namespace parser
