#include "transformer.hpp"
#include "minizinc/ast.hh"
#include "utils.hpp"

#include <fmt/base.h>
#include <fmt/format.h>
#include <minizinc/flatten_internal.hh>
#include <minizinc/type.hh>

#include <algorithm>
#include <functional>
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
  case MiniZinc::Type::BT_BOT: {
    return ast::types::Unspecified{};
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

void make_reif_call(MiniZinc::Call* call, Transformer::Stack::Scope& scope) {
  std::string id{call->id().c_str()};
  std::vector args(call->args().begin(), call->args().end());

  auto bool_arg =
      new MiniZinc::Id(MiniZinc::Location(), "\x1\x1\x1\x0", nullptr);
  scope.add(ast::DeclVariable{
      .id = std::string{"\x1\x1\x1\x0"},
      .var_type = ast::types::Bool{},
      .domain = std::nullopt,
      .value = std::nullopt,
  });
  bool_arg->type(MiniZinc::Type::varbool());
  args.push_back(bool_arg);

  call->id(MiniZinc::ASTString(id + "_reif"));
  call->args(args);
}

auto make_ite_call(MiniZinc::ITE& ite, MiniZinc::EnvI& env,
                   Transformer::Stack::Scope& scope) -> MiniZinc::Call* {
  std::vector conditions =
      std::views::iota(0u, ite.size()) |
      std::views::transform([&](auto i) { return ite.ifExpr(i); }) |
      std::ranges::to<std::vector>();

  std::vector branches =
      std::views::iota(0u, ite.size()) |
      std::views::transform([&](auto i) { return ite.thenExpr(i); }) |
      std::ranges::to<std::vector>();

  if (conditions.back() != env.constants.literalTrue) {
    conditions.emplace_back(env.constants.literalTrue);
    branches.push_back(ite.elseExpr());
  }

  auto* conditions_literal =
      new MiniZinc::ArrayLit(MiniZinc::Location().introduce(), conditions);
  MiniZinc::Type conditions_t = MiniZinc::Type::arrType(
      env, MiniZinc::Type::bot(1), MiniZinc::Type::varbool(1));

  conditions_t.ti(MiniZinc::Type::TI_VAR);
  conditions_literal->type(conditions_t);

  auto* branches_literal =
      new MiniZinc::ArrayLit(MiniZinc::Location().introduce(), branches);
  MiniZinc::Type branches_t = MiniZinc::Type::arrType(
      env, MiniZinc::Type::bot(1), MiniZinc::Expression::type(branches[0]));

  branches_t.ti(MiniZinc::Type::TI_VAR);
  branches_literal->type(branches_t);

  auto id_arg = new MiniZinc::Id(MiniZinc::Location(), "ite_result_2137", nullptr);
  id_arg->type(MiniZinc::Expression::type(branches[0]));

  scope.add(ast::DeclVariable{
      .id = std::string{"ite_result_2137"},
      .var_type = resolve_base_type(id_arg->type().bt()),
      .domain = std::nullopt,
      .value = std::nullopt,
  });

  auto ite_pred =
      MiniZinc::Call::a(MiniZinc::Expression::loc(&ite).introduce(),
                        MiniZinc::ASTString("if_then_else"),
                        {conditions_literal, branches_literal, id_arg});

  MiniZinc::Expression::cast<MiniZinc::Call>(ite_pred)->decl(env.model->matchFn(
      env, MiniZinc::Expression::cast<MiniZinc::Call>(ite_pred), false, true));
  return ite_pred;
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
                        return map_ptr(inner->domain());
                      return std::nullopt;
                    }) |
                std::ranges::to<std::vector>()};
  }

  MiniZinc::Type type = type_inst->type();
  type.dim(0);

  return map(type);
}

auto Transformer::map(MiniZinc::Type const& type) -> ast::Type {
  if (type.dim() > 0)
    return ast::types::Array{};

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
                          [](ast::types::Unspecified const&) -> ast::Type {
                            return ast::types::UnspecifiedSet{};
                          },
                          [](auto const& t) -> ast::Type { return t; }},
        resolve_base_type(type.bt()));

  return resolve_base_type(type.bt());
}

auto Transformer::handle_const_decl(MiniZinc::VarDecl* var_decl)
    -> ast::VarDecl {
  return ast::DeclConst{.id = std::string{var_decl->id()->v().c_str()},
                        .type = map(var_decl->ti()),
                        .value = var_decl->e()
                                   ? std::optional{map_ptr(var_decl->e())}
                                   : std::nullopt};
}

auto Transformer::handle_var_decl(MiniZinc::VarDecl* var_decl) -> ast::VarDecl {
  return ast::DeclVariable{
      .id = std::string{var_decl->id()->v().c_str()},
      .var_type = map(var_decl->ti()),
      .domain = var_decl->ti()->domain() != nullptr &&
                        MiniZinc::Expression::eid(var_decl->ti()->domain()) !=
                            MiniZinc::TIId::eid
                  ? std::optional{map_ptr(var_decl->ti()->domain())}
                  : std::nullopt,
      .value =
          var_decl->e() ? std::optional{map_ptr(var_decl->e())} : std::nullopt};
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

  if (is_global && std::holds_alternative<ast::DeclConst>(var)) {
    auto& const_decl = std::get<ast::DeclConst>(var);
    if (!const_decl.value) {
      const_decl.value = ast::ptr(ast::Call{
          .id = "load_from_json",
          .args = {ast::ptr(ast::LiteralString{const_decl.id})},
          .expr_type = utils::var_type(var),
          .is_var = false,
      });
    }
  }

  if (is_global)
    stack.variable_map[utils::var_id(var)].push_back(var);

  return var;
}

auto Transformer::map(MiniZinc::ArrayLit* arr_lit) -> ast::Expr {
  return ast::LiteralArray{
      .value = arr_lit->getVec() | std::views::transform([&](auto& array_expr) {
                 return map_ptr(array_expr);
               }) |
               std::ranges::to<std::vector>(),
      .expr_type = map(arr_lit->type()),
      .is_var = arr_lit->type().isvar()};
}

auto Transformer::map(MiniZinc::SetLit* set_lit) -> ast::Expr {
  if (auto* isv = set_lit->isv(); isv != nullptr) {
    auto const& min = isv->min();
    auto const& max = isv->max();

    assert(min.isFinite());
    assert(max.isFinite());

    return ast::BinOp{.kind = ast::BinOp::OpKind::DOTDOT,
                      .lhs = ast::ptr(ast::LiteralInt{min.toInt()}),
                      .rhs = ast::ptr(ast::LiteralInt{max.toInt()}),
                      .expr_type = ast::types::IntSet{},
                      .is_var = false};
  }

  return ast::LiteralSet{.value = set_lit->v() |
                                  std::views::transform([&](auto& set_expr) {
                                    return map_ptr(set_expr);
                                  }) |
                                  std::ranges::to<std::vector>(),
                         .expr_type = map(set_lit->type()),
                         .is_var = set_lit->type().isvar()};
}

auto Transformer::map(MiniZinc::Comprehension* comp) -> ast::Comprehension {
  std::vector<ast::Generator> generators;

  auto scope = stack.scope();

  for (auto const i : std::views::iota(0u, comp->numberOfGenerators())) {

    assert((comp->in(i) != nullptr || comp->where(i) != nullptr) &&
           "Null generator");

    if (comp->in(i) != nullptr) {
      ast::ExprHandle const in_expr = map_ptr(comp->in(i));

      for (auto const j : std::views::iota(0u, comp->numberOfDecls(i))) {
        ast::VarDecl decl_expr = handle_const_decl(comp->decl(i, j));
        assert(std::holds_alternative<ast::DeclConst>(decl_expr));

        scope.add(decl_expr);

        generators.push_back(ast::Iterator{
            .variable = std::get<ast::DeclConst>(std::move(decl_expr)),
            .in = in_expr});
      }
    }

    if (comp->where(i) != nullptr)
      generators.push_back(map_ptr(comp->where(i)));
  }

  return ast::Comprehension{.body = map_ptr(comp->e()),
                            .generators = std::move(generators),
                            .expr_type = map(comp->type()),
                            .is_var = comp->type().isvar()};
}

auto Transformer::map(MiniZinc::Call* call) -> ast::Expr {
  assert(call->id().c_str() != nullptr && "Function call: null function id");
  auto const old_id = std::string{call->id().c_str()};
  std::vector old_args(call->args().begin(), call->args().end());
  auto const id = reformat_id(old_id);
  assert(!id.empty() && "Function call: empty function id");

  auto const function_item = model.matchFn(env, call, true, false);
  save(function_item);

  auto const result = ast::Call{
      .id = id,
      .args = call->args() |
              std::views::transform([&](auto& arg) { return map_ptr(arg); }) |
              std::ranges::to<std::vector>(),
      .expr_type = map(function_item->ti()),
      .is_var = function_item->ti()->type().isvar()};

  auto scope = stack.scope();
  make_reif_call(call, scope);
  auto const function_item_reif = model.matchFn(env, call, true, false);
  if (function_item_reif != nullptr)
    save(function_item_reif);

  call->id(MiniZinc::ASTString(old_id));
  call->args(old_args);

  return result;
}

auto Transformer::map(MiniZinc::ArrayAccess* array_access) -> ast::Expr {
  std::vector indexes =
      array_access->idx() |
      std::views::transform([&](auto& ix) { return map_ptr(ix); }) |
      std::ranges::to<std::vector>();

  ast::Expr arr = map(array_access->v());

  bool const is_index_var_type =
      std::any_of(indexes.begin(), indexes.end(),
                  [](auto const& ix) { return utils::is_expr_var(*ix); });

  return ast::ArrayAccess{.arr = ast::ptr(std::move(arr)),
                          .indexes = std::move(indexes),
                          .expr_type = map(array_access->type()),
                          .is_var = array_access->type().isvar(),
                          .is_index_var_type = is_index_var_type};
}

void Transformer::save(MiniZinc::FunctionI* function) {
  if (function->e() == nullptr)
    return;

  auto const id = reformat_id(std::string{function->id().c_str()});

  if (functions.find(id) != functions.end())
    return;

  functions.emplace(id, ast::Function{});

  auto scope = stack.scope();

  std::vector<ast::IdExpr> params;
  for (auto const ix : std::views::iota(0u, function->paramCount())) {
    std::optional const var = map(function->param(ix), false, false);
    assert("Parameter of a function should be a valid var decl" && var);

    scope.add(*var);

    params.push_back(ast::IdExpr::from_var(*var));
  }

  auto& fn = functions[id];
  fn.id = id;
  fn.params = params;
  fn.body = map_ptr(function->e());
}

auto Transformer::map(MiniZinc::Expression* expr) -> ast::Expr {
  switch (MiniZinc::Expression::eid(expr)) {
  case MiniZinc::IntLit::eid: {
    auto const* int_lit = MiniZinc::Expression::cast<MiniZinc::IntLit>(expr);
    auto const value = MiniZinc::IntLit::v(int_lit);

    assert(value.isFinite());

    return ast::LiteralInt{value.toInt()};
  }
  case MiniZinc::Id::eid: {
    auto* id = MiniZinc::Expression::cast<MiniZinc::Id>(expr);
    assert(id->v().c_str() != nullptr && id->v() != "" &&
           "Id Expression: null id");

    std::string id_str{id->v().c_str()};
    auto const& var = stack.variable_map[id_str].back();

    return ast::IdExpr::from_var(var);
  }
  case MiniZinc::BinOp::eid: {
    auto* bin_op = MiniZinc::Expression::cast<MiniZinc::BinOp>(expr);
    return map(bin_op);
  }
  case MiniZinc::UnOp::eid: {
    auto* un_op = MiniZinc::Expression::cast<MiniZinc::UnOp>(expr);
    return map(un_op);
  }
  case MiniZinc::Call::eid: {
    auto* call = MiniZinc::Expression::cast<MiniZinc::Call>(expr);
    return map(call);
  }
  case MiniZinc::FloatLit::eid: {
    auto const* float_lit =
        MiniZinc::Expression::cast<MiniZinc::FloatLit>(expr);
    auto const value = MiniZinc::FloatLit::v(float_lit);

    assert(value.isFinite());

    return ast::LiteralFloat{value.toDouble()};
  }
  case MiniZinc::SetLit::eid: {
    auto* set_lit = MiniZinc::Expression::cast<MiniZinc::SetLit>(expr);
    return map(set_lit);
  }
  case MiniZinc::BoolLit::eid: {
    auto* bool_lit = MiniZinc::Expression::cast<MiniZinc::BoolLit>(expr);
    return ast::LiteralBool{bool_lit->v()};
  }
  case MiniZinc::StringLit::eid: {
    auto* string_lit = MiniZinc::Expression::cast<MiniZinc::StringLit>(expr);
    return ast::LiteralString{string_lit->v().c_str() != nullptr
                                  ? std::string{string_lit->v().c_str()}
                                  : std::string{}};
  }
  case MiniZinc::ArrayLit::eid: {
    auto* array_lit = MiniZinc::Expression::cast<MiniZinc::ArrayLit>(expr);
    return map(array_lit);
  }
  case MiniZinc::Comprehension::eid: {
    auto* comp = MiniZinc::Expression::cast<MiniZinc::Comprehension>(expr);
    return map(comp);
  }
  case MiniZinc::ArrayAccess::eid: {
    auto* array_access =
        MiniZinc::Expression::cast<MiniZinc::ArrayAccess>(expr);

    return map(array_access);
  }
  case MiniZinc::ITE::eid: {
    auto* ite = MiniZinc::Expression::cast<MiniZinc::ITE>(expr);
    return map(ite);
  }
  case MiniZinc::Let::eid: {
    auto* let = MiniZinc::Expression::cast<MiniZinc::Let>(expr);
    return map(let);
  }
  case MiniZinc::VarDecl::eid:
    assert(false && "Should not reach var decl through this function");
  case MiniZinc::AnonVar::eid:
    fmt::println("E_ANON");
    assert(false);
  case MiniZinc::FieldAccess::eid:
    fmt::println("E_FIELDACCESS");
    assert(false);
  case MiniZinc::TypeInst::eid:
    fmt::println("E_TI");
    assert(false);
  case MiniZinc::TIId::eid:
    fmt::println("E_TIID");
    assert(false);
  default:
    assert(false);
  }
}

auto Transformer::map(MiniZinc::BinOp* bin_op) -> ast::Expr {
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
    case MiniZinc::BOT_DIV:
      return ast::BinOp::OpKind::DIV;
    case MiniZinc::BOT_POW:
      return ast::BinOp::OpKind::POW;
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
    case MiniZinc::BOT_XOR:
      return ast::BinOp::OpKind::XOR;
    case MiniZinc::BOT_IMPL:
      return ast::BinOp::OpKind::IMPL;
    case MiniZinc::BOT_RIMPL:
      return ast::BinOp::OpKind::RIMPL;
    case MiniZinc::BOT_IN:
      return ast::BinOp::OpKind::IN;
    case MiniZinc::BOT_EQUIV:
      return ast::BinOp::OpKind::EQUIV;
    case MiniZinc::BOT_DIFF:
      return ast::BinOp::OpKind::DIFF;
    case MiniZinc::BOT_INTERSECT:
      return ast::BinOp::OpKind::INTERSECT;
    case MiniZinc::BOT_UNION:
      return ast::BinOp::OpKind::UNION;
    case MiniZinc::BOT_SYMDIFF:
      return ast::BinOp::OpKind::SYMDIFF;
    case MiniZinc::BOT_SUBSET:
      return ast::BinOp::OpKind::SUBSET;
    case MiniZinc::BOT_SUPERSET:
      return ast::BinOp::OpKind::SUPERSET;
    default:
      assert(false);
    }
  };

  ast::Expr lhs = map(bin_op->lhs());
  ast::Expr rhs = map(bin_op->rhs());

  return ast::BinOp{.kind = match_kind(bin_op->op()),
                    .lhs = ast::ptr(std::move(lhs)),
                    .rhs = ast::ptr(std::move(rhs)),
                    .expr_type = map(bin_op->type()),
                    .is_var = bin_op->type().isvar()};
}

auto Transformer::map_ptr(MiniZinc::Expression* e) -> ast::ExprHandle {
  return ast::ptr(map(e));
}

auto Transformer::map(MiniZinc::UnOp* un_op) -> ast::Expr {
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

  ast::Expr expr = map(un_op->e());

  return ast::UnaryOp{.kind = match_kind(un_op->op()),
                      .expr = ast::ptr(std::move(expr)),
                      .expr_type = map(un_op->type()),
                      .is_var = un_op->type().isvar()};
}

auto Transformer::map(MiniZinc::ITE* ite) -> ast::Expr {
  auto r =
      std::views::iota(0u, ite->size()) | std::views::transform([&](auto i) {
        return MiniZinc::Expression::type(ite->ifExpr(i)).isvar();
      });
  if (std::any_of(r.begin(), r.end(), std::identity{})) {
    auto scope = stack.scope();
    return map(make_ite_call(*ite, env, scope));
  }

  std::vector if_then =
      std::views::iota(0u, ite->size()) |
      std::views::transform(
          [&](auto i) -> std::pair<ast::ExprHandle, ast::ExprHandle> {
            return {map_ptr(ite->ifExpr(i)), map_ptr(ite->thenExpr(i))};
          }) |
      std::ranges::to<std::vector>();

  std::optional else_expr =
      ite->elseExpr() ? map_ptr(ite->elseExpr()) : nullptr;

  return ast::IfThenElse{.if_then = std::move(if_then),
                         .else_expr = std::move(else_expr),
                         .expr_type = map(ite->type()),
                         .is_var = ite->type().isvar()};
}

auto Transformer::map(MiniZinc::Let* let) -> ast::Expr {
  let_in_ctr++;
  std::string id = fmt::format("let_in_{}", let_in_ctr);

  auto scope = stack.scope();

  auto is_var_decl = [](auto& expr) {
    return MiniZinc::Expression::eid(expr) == MiniZinc::VarDecl::eid;
  };

  std::vector<ast::VarDecl> args =
      let->let() | std::views::filter(is_var_decl) |
      std::views::transform([&](auto& expr) {
        auto var = map(MiniZinc::Expression::cast<MiniZinc::VarDecl>(expr),
                       false, false);
        assert(var);
        scope.add(*var);

        return *var;
      }) |
      std::ranges::to<std::vector>();

  std::vector<ast::ExprHandle> constraints =
      let->let() | std::views::filter(std::not_fn(is_var_decl)) |
      std::views::transform([&](auto& expr) { return map_ptr(expr); }) |
      std::ranges::to<std::vector>();

  ast::ExprHandle function_body = map_ptr(let->in());
  functions.emplace(
      id, ast::Function{.id = id,
                        .params = args |
                                  std::views::transform(ast::IdExpr::from_var) |
                                  std::ranges::to<std::vector>(),
                        .body = function_body});

  return ast::LetIn{.id = id,
                    .declarations = std::move(args),
                    .constraints = std::move(constraints),
                    .expr_type = utils::expr_type(*function_body),
                    .is_var = utils::is_expr_var(*function_body)};
}

auto Transformer::map(MiniZinc::SolveI* solve_item) -> ast::SolveType {
  if (!solve_item)
    return ast::solve_type::Sat{};

  switch (solve_item->st()) {
  case MiniZinc::SolveI::SolveType::ST_MAX:
    return ast::solve_type::Max{.expr = ast::ptr(find_objective_expr())};
  case MiniZinc::SolveI::SolveType::ST_MIN:
    return ast::solve_type::Min{.expr = ast::ptr(find_objective_expr())};
  case MiniZinc::SolveI::SolveType::ST_SAT:
    return ast::solve_type::Sat{};
  }
  assert(false);
}

auto Transformer::find_objective_expr() -> ast::Expr {
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

Transformer::Stack::Scope::Scope(ast::VariableMap& pMap) : map{pMap} {}

void Transformer::Stack::Scope::add(ast::VarDecl const& var) {
  std::string s = utils::var_id(var);
  map[s].push_back(var);
  decls.push_back(s);
}

Transformer::Stack::Scope::~Scope() {
  for (auto& decl : decls) {
    map[decl].pop_back();
  }
}

auto Transformer::Stack::scope() -> Stack::Scope { return Scope{variable_map}; }

} // namespace parser
