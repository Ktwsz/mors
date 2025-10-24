// TODO
// assert that we do not have floats
// make mapping more monadic with expected
#include "transformer.hpp"
#include "ast.hpp"
#include "utils.hpp"

#include <algorithm>
#include <fmt/base.h>
#include <fmt/format.h>
#include <minizinc/type.hh>

#include <iterator>
#include <optional>
#include <ranges>
#include <string_view>
#include <utility>
#include <variant>

namespace parser {
namespace {

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
    return ast::types::Array{
        .dims = type_inst->ranges() |
                std::views::transform(
                    [&](auto& inner) -> std::optional<ast::ExprHandle> {
                      if (inner->domain() &&
                          MiniZinc::Expression::eid(inner->domain()) !=
                              MiniZinc::TIId::eid)
                        return map(inner->domain());
                      return std::nullopt;
                    }) |
                std::ranges::to<std::vector>()};
  }

  auto const& type = type_inst->type();
  if (type.isSet())
    return std::visit(
        utils::overloaded{[](ast::types::Int const&) -> ast::Type {
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
  return ast::DeclConst{
      .id = std::string{var_decl->id()->v().c_str()},
      .type = map(var_decl->ti()),
      .value = var_decl->e()
                 ? std::optional<ast::ExprHandle>{map(var_decl->e())}
                 : std::nullopt};
}

auto Transformer::handle_var_decl(MiniZinc::VarDecl* var_decl) -> ast::VarDecl {
  return ast::DeclVariable{
      .id = std::string{var_decl->id()->v().c_str()},
      .var_type = map(var_decl->ti()),
      .domain =
          var_decl->ti()->domain() != nullptr &&
                  MiniZinc::Expression::eid(var_decl->ti()->domain()) !=
                      MiniZinc::TIId::eid
              ? std::optional<ast::ExprHandle>{map(var_decl->ti()->domain())}
              : std::nullopt,
      .value = var_decl->e()
                 ? std::optional<ast::ExprHandle>{map(var_decl->e())}
                 : std::nullopt};
}

auto Transformer::map(MiniZinc::VarDecl* var_decl, bool const is_global,
                      bool const check_id) -> std::optional<ast::VarDecl> {
  if (check_id &&
      (!var_decl->item()->loc().filename().endsWith(opts.model_path) ||
       var_decl->id()->str() == "_objective"))
    return std::nullopt;

  auto var = MiniZinc::Expression::type(var_decl->ti()).isPar()
               ? handle_const_decl(var_decl)
               : handle_var_decl(var_decl);

  std::visit(utils::overloaded{
                 [&](ast::DeclVariable& var) { var.is_global = is_global; },
                 [&](ast::DeclConst& var) { var.is_global = is_global; },
             },
             var);

  if (auto const json_path = opts.isInputInJson(utils::var_id(var));
      json_path && is_global) {
    auto var_value = utils::var_value(var);
    if (std::holds_alternative<ast::Call>(*var_value)) {
      auto& call = std::get<ast::Call>(*var_value);
      call.args.back() = std::make_shared<ast::Expr>(ast::Call{
          .id = "load_from_json",
          .args = {std::make_shared<ast::Expr>(
              ast::LiteralString{.value = *json_path})},
          .expr_type = utils::var_type(var),
          .is_var = false,
      });
    }
  }

  variable_map[utils::var_id(var)].push_back(var);

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
        .expr_type = ast::types::IntSet{},
        .is_var = false});
  }

  return std::make_shared<ast::Expr>(ast::LiteralSet{
      .value = set_lit->v() | std::views::transform([&](auto& set_expr) {
                 return map(set_expr);
               }) |
               std::ranges::to<std::vector>()});
}

auto Transformer::map(MiniZinc::Comprehension* comp) -> ast::Comprehension {
  std::vector<ast::Generator> generators;
  std::vector<std::string> decls_to_pop;

  for (auto const i : std::views::iota(0u, comp->numberOfGenerators())) {

    assert((comp->in(i) != nullptr || comp->where(i) != nullptr) &&
           "Null generator");

    if (comp->in(i) != nullptr) {
      ast::ExprHandle const in_expr = map(comp->in(i));

      for (auto const j : std::views::iota(0u, comp->numberOfDecls(i))) {
        ast::VarDecl decl_expr = handle_const_decl(comp->decl(i, j));
        assert(std::holds_alternative<ast::DeclConst>(decl_expr));

        std::string const& tmp_id = std::get<ast::DeclConst>(decl_expr).id;
        decls_to_pop.push_back(tmp_id);
        variable_map[tmp_id].push_back(decl_expr);

        generators.push_back(ast::Iterator{
            .variable = std::get<ast::DeclConst>(std::move(decl_expr)),
            .in = in_expr});
      }
    }

    if (comp->where(i) != nullptr)
      generators.push_back(map(comp->where(i)));
  }

  ast::ExprHandle body = map(comp->e());
  bool const is_body_var = utils::is_expr_var(*body);

  for (auto const& s : decls_to_pop)
    variable_map[s].pop_back();

  return ast::Comprehension{.body = std::move(body),
                            .generators = std::move(generators),
                            .is_var = is_body_var};
}

auto Transformer::map(MiniZinc::Call* call) -> ast::ExprHandle {
  assert(call->id().c_str() != nullptr && "Function call: null function id");
  auto const id = reformat_id(std::string{call->id().c_str()});
  assert(!id.empty() && "Function call: empty function id");

  auto const function_item = model.matchFn(env, call, true, false);
  save(function_item);

  return std::make_shared<ast::Expr>(ast::Call{
      .id = id,
      .args = call->args() |
              std::views::transform([&](auto& arg) { return map(arg); }) |
              std::ranges::to<std::vector>(),
      .expr_type = map(function_item->ti()),
      .is_var = function_item->ti()->type().isvar()});
}

auto Transformer::map(MiniZinc::ArrayAccess* array_access) -> ast::ExprHandle {
  ast::ArrayAccess ast_array{
      .arr = map(array_access->v()),
      .indexes = array_access->idx() |
                 std::views::transform([&](auto& ix) { return map(ix); }) |
                 std::ranges::to<std::vector>(),

      .is_var = false};

  ast_array.is_var = utils::is_expr_var(*ast_array.arr);

  ast_array.is_index_var_type =
      std::any_of(ast_array.indexes.begin(), ast_array.indexes.end(),
                  [](auto const& ix) { return utils::is_expr_var(*ix); });
  ast_array.is_var = ast_array.is_var || ast_array.is_index_var_type;

  return std::make_shared<ast::Expr>(ast_array);
}

void Transformer::save(MiniZinc::FunctionI* function) {
  if (function->id().endsWith("abs")) // TODO - identify BIFs
    return;
  if (function->e() == nullptr)
    return;

  std::vector<std::string> decls_to_pop;

  auto const id = reformat_id(std::string{function->id().c_str()});

  std::vector<ast::IdExpr> params;
  for (auto const ix : std::views::iota(0u, function->paramCount())) {
    std::optional<ast::VarDecl> const var =
        map(function->param(ix), false, false);
    assert("Parameter of a function should be a valid var decl" && var);

    std::string id = utils::var_id(*var);

    decls_to_pop.push_back(id);

    params.push_back(ast::IdExpr::from_var(id, *var));
  }

  ast::ExprHandle function_body = map(function->e());

  for (auto const& s : decls_to_pop)
    variable_map[s].pop_back();

  functions.emplace(id, ast::Function{id, params, std::move(function_body)});
}

auto Transformer::map(MiniZinc::Expression* expr) -> ast::ExprHandle {
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

    return std::make_shared<ast::Expr>(ast::IdExpr::from_var(id_str, var));
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
    return map(call);
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
    auto ast_array = ast::LiteralArray{};
    for (auto array_expr : array_lit->getVec())
      ast_array.value.push_back(map(array_expr));

    return std::make_shared<ast::Expr>(ast_array);
  }
  case MiniZinc::Expression::E_COMP: {
    auto* comp = MiniZinc::Expression::cast<MiniZinc::Comprehension>(expr);
    return std::make_shared<ast::Expr>(map(comp));
  }
  case MiniZinc::Expression::E_ARRAYACCESS: {
    auto* array_access =
        MiniZinc::Expression::cast<MiniZinc::ArrayAccess>(expr);

    return map(array_access);
  }
  case MiniZinc::Expression::E_ITE: {
    auto* ite = MiniZinc::Expression::cast<MiniZinc::ITE>(expr);
    return map(ite);
  }
  case MiniZinc::Expression::E_VARDECL: {
    assert(false && "Should not reach var decl through this function");
  }
  case MiniZinc::Expression::E_ANON:
    fmt::println("E_ANON");
    assert(false);
  case MiniZinc::Expression::E_FIELDACCESS:
    fmt::println("E_FIELDACCESS");
    assert(false);
  case MiniZinc::Expression::E_LET: {
    auto* let = MiniZinc::Expression::cast<MiniZinc::Let>(expr);
    return map(let);
  }
  case MiniZinc::Expression::E_TI:
    fmt::println("E_TI");
    assert(false);
  case MiniZinc::Expression::E_TIID:
    fmt::println("E_TIID");
    assert(false);
  default:
    assert(false);
  }
}

namespace {
auto is_bool_operator(MiniZinc::BinOpType const t) -> bool {
  switch (t) {
  case MiniZinc::BOT_NQ:
  case MiniZinc::BOT_EQ:
  case MiniZinc::BOT_GQ:
  case MiniZinc::BOT_GR:
  case MiniZinc::BOT_LE:
  case MiniZinc::BOT_LQ:
  case MiniZinc::BOT_AND:
  case MiniZinc::BOT_OR:
  case MiniZinc::BOT_IMPL:
  case MiniZinc::BOT_IN:
  case MiniZinc::BOT_EQUIV:
  case MiniZinc::BOT_RIMPL:
  case MiniZinc::BOT_XOR:
    return true;
  case MiniZinc::BOT_DOTDOT:
  case MiniZinc::BOT_PLUSPLUS:
  case MiniZinc::BOT_PLUS:
  case MiniZinc::BOT_MINUS:
  case MiniZinc::BOT_MULT:
  case MiniZinc::BOT_IDIV:
  case MiniZinc::BOT_DIV:
  case MiniZinc::BOT_POW:
  case MiniZinc::BOT_SUBSET:
  case MiniZinc::BOT_SUPERSET:
  case MiniZinc::BOT_UNION:
  case MiniZinc::BOT_DIFF:
  case MiniZinc::BOT_SYMDIFF:
  case MiniZinc::BOT_INTERSECT:
  case MiniZinc::BOT_MOD:
    return false;
  }
}
} // namespace

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
    case MiniZinc::BOT_IMPL:
      return ast::BinOp::OpKind::IMPL;
    case MiniZinc::BOT_IN:
      return ast::BinOp::OpKind::IN;
    case MiniZinc::BOT_EQUIV:
      return ast::BinOp::OpKind::EQUIV;
    default:
      assert(false);
    }
  };

  ast::ExprHandle lhs = map(bin_op->lhs());
  ast::ExprHandle rhs = map(bin_op->rhs());

  ast::Type expr_type = is_bool_operator(bin_op->op()) ? ast::types::Bool{}
                                                       : utils::expr_type(*lhs);
  bool const is_var = utils::is_expr_var(*lhs) || utils::is_expr_var(*rhs);

  return std::make_shared<ast::Expr>(
      ast::BinOp{.kind = match_kind(bin_op->op()),
                 .lhs = std::move(lhs),
                 .rhs = std::move(rhs),
                 .expr_type = std::move(expr_type),
                 .is_var = is_var});
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

  ast::ExprHandle expr = map(un_op->e());

  auto expr_type = utils::expr_type(*expr);
  bool const is_var = utils::is_expr_var(*expr);

  return std::make_shared<ast::Expr>(
      ast::UnaryOp{.kind = match_kind(un_op->op()),
                   .expr = std::move(expr),
                   .expr_type = std::move(expr_type),
                   .is_var = is_var});
}

auto Transformer::map(MiniZinc::ITE* ite) -> ast::ExprHandle {
  auto result = ast::IfThenElse{
      .if_then =
          std::views::iota(0u, ite->size()) |
          std::views::transform(
              [&](auto i) -> std::pair<ast::ExprHandle, ast::ExprHandle> {
                return {map(ite->ifExpr(i)), map(ite->thenExpr(i))};
              }) |
          std::ranges::to<std::vector>(),
      .else_expr = ite->elseExpr() ? map(ite->elseExpr()) : nullptr,
      .expr_type = ast::types::Int{}};

  result.is_var =
      std::any_of(result.if_then.begin(), result.if_then.end(),
                  [](auto& i) { return utils::is_expr_var(*i.second); }) ||
      result.else_expr
          .transform(
              [](auto& else_expr) { return utils::is_expr_var(*else_expr); })
          .value_or(false);

  result.expr_type = utils::expr_type(*result.if_then.front().second);

  return std::make_shared<ast::Expr>(std::move(result));
}

auto Transformer::map(MiniZinc::Let* let) -> ast::ExprHandle {
  let_in_ctr++;

  std::vector<ast::VarDecl> args;
  std::vector<ast::ExprHandle> constraints;

  for (auto const& expr : let->let()) {
    if (MiniZinc::Expression::eid(expr) == MiniZinc::VarDecl::eid) {
      std::optional<ast::VarDecl> ast_var = map(
          MiniZinc::Expression::cast<MiniZinc::VarDecl>(expr), false, false);
      assert(ast_var);
      args.push_back(std::move(*ast_var));
      continue;
    }

    constraints.push_back(map(expr));
  }

  ast::ExprHandle const function_body = map(let->in());
  std::string id = fmt::format("in_{}", let_in_ctr);
  functions.emplace(
      id, ast::Function{
              id, args | std::views::transform([](ast::VarDecl const& var) {
                    return ast::IdExpr::from_var(utils::var_id(var), var);
                  }) | std::ranges::to<std::vector>(),
              function_body});

  for (auto const& s : args)
    variable_map[utils::var_id(s)].pop_back();

  return std::make_shared<ast::Expr>(
      ast::LetIn{.id = fmt::format("{}", let_in_ctr),
                 .declarations = std::move(args),
                 .constraints = std::move(constraints),
                 .expr_type = utils::expr_type(*function_body),
                 .is_var = utils::is_expr_var(*function_body)});
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

    return map(var_decl->e());
  }

  assert(false);
}

} // namespace parser
