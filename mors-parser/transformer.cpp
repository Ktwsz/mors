#include "transformer.hpp"
#include "parsing_errors.hpp"
#include "utils.hpp"

#include <minizinc/ast.hh>
#include <minizinc/flatten_internal.hh>
#include <minizinc/type.hh>

#include <algorithm>
#include <format>
#include <functional>
#include <optional>
#include <print>
#include <ranges>
#include <string_view>
#include <utility>
#include <variant>

namespace parser {
namespace {

auto resolve_base_type(MiniZinc::Type::BaseType base_type) -> ast::Type {
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
    return ast::types::Int{};
  }
  case MiniZinc::Type::BT_BOT: {
    return ast::types::Unspecified{};
  }
  default:
    throw err::Unsupported{
        .message =
            "Unsupported type: " + [](auto const base_type) -> std::string {
          switch (base_type) {
          case MiniZinc::Type::BT_ANN:
            return "Annotation";
          case MiniZinc::Type::BT_TUPLE:
            return "Tuple";
          case MiniZinc::Type::BT_RECORD:
            return "Record";
          case MiniZinc::Type::BT_UNKNOWN:
            return "Unknown";
          default:
            assert(false);
          }
        }(base_type),
    };
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

  auto id_arg =
      new MiniZinc::Id(MiniZinc::Location(), "ite_result_2137", nullptr);
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

auto make_location(MiniZinc::Location const& loc)
    -> err::Unsupported::Location {
  return err::Unsupported::Location{
      .filename = loc.filename().substr(),
      .first_line = loc.firstLine(),
      .first_column = loc.firstColumn(),
      .last_line = loc.lastLine(),
      .last_column = loc.lastColumn(),
  };
}

auto is_any_type_opt(std::vector<MiniZinc::Type> const& types) {
  return std::any_of(types.begin(), types.end(),
                     [](auto const& t) { return t.ot(); });
}

auto deopt_types(std::vector<MiniZinc::Type> const& types)
    -> std::vector<MiniZinc::Type> {
  std::vector new_types(types);
  for (auto& t : new_types) {
    t.ot(MiniZinc::Type::OptType::OT_PRESENT);
  }

  return new_types;
}

} // namespace

auto Transformer::map(MiniZinc::TypeInst* type_inst) -> ast::Type {
  MiniZinc::Type type = type_inst->type();
  auto inner_type = map(type);

  if (!type_inst->isarray())
    return inner_type;

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
              std::ranges::to<std::vector>(),
      .inner_type =
          type_inst->type().dim() > 0
              ? std::move(std::get<ast::types::Array>(inner_type).inner_type)
              : std::make_shared<ast::Type>(std::move(inner_type))};
}

auto Transformer::map(MiniZinc::Type const& type) -> ast::Type {
  ast::Type base_type;

  if (type.st() == MiniZinc::Type::ST_SET)
    base_type = std::visit(utils::overloaded{
                               [](ast::types::Int const&) -> ast::Type {
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
                               [](auto const& t) -> ast::Type { return t; },
                           },
                           resolve_base_type(type.bt()));
  else
    base_type = resolve_base_type(type.bt());

  if (type.dim() > 0)
    return ast::types::Array{
        .dims = {},
        .inner_type = std::make_shared<ast::Type>(base_type),
    };
  return base_type;
}

auto Transformer::handle_const_decl(MiniZinc::VarDecl* var_decl)
    -> ast::VarDecl {
  return ast::DeclConst{
      .id = var_decl->id()->v().substr(),
      .type = map(var_decl->ti()),
      .value = map_opt_ptr(var_decl->e()),
  };
}

auto Transformer::handle_var_decl(MiniZinc::VarDecl* var_decl) -> ast::VarDecl {
  ast::Type type = map(var_decl->ti());

  if (utils::is_unsupported_var_type(type))
    throw err::Unsupported{
        .location = {make_location(MiniZinc::Expression::loc(var_decl))},
        .message = "Variable of type " + utils::type_to_string(type),
    };

  return ast::DeclVariable{
      .id = var_decl->id()->v().substr(),
      .var_type = std::move(type),
      .domain = var_decl->ti()->domain() &&
                        MiniZinc::Expression::eid(var_decl->ti()->domain()) !=
                            MiniZinc::TIId::eid
                  ? map_opt_ptr(var_decl->ti()->domain())
                  : std::nullopt,
      .value = map_opt_ptr(var_decl->e()),
  };
}

// TODO use epxlicit bools
auto Transformer::map(MiniZinc::VarDecl* var_decl, bool const is_global,
                      bool const check_id, bool const ignore_optional)
    -> ast::VarDecl {
  if (check_id &&
      (!var_decl->item()->loc().filename().endsWith(opts.model_file) ||
       var_decl->id()->str() == "_objective"))
    throw Ignore{};

  if (!ignore_optional && var_decl->type().isOpt())
    throw err::Unsupported{
        .location = {make_location(MiniZinc::Expression::loc(var_decl))},
        .message = "Optional type",
    };

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
      bool const is_decl_array =
          std::holds_alternative<ast::types::Array>(const_decl.type);
      bool const is_decl_set = utils::is_type_set(const_decl.type);
      std::string call_id = is_decl_array ? "load_array_from_json"
                          : is_decl_set   ? "load_set_from_json"
                                          : "load_from_json";

      const_decl.value = ast::ptr(ast::Call{
          .id = call_id,
          .args = {ast::ptr(ast::LiteralString{const_decl.id})},
          .expr_type = utils::var_type(var),
          .is_var = false,
      });

      if (is_decl_array) {
        auto& args = std::get<ast::Call>(**const_decl.value).args;
        auto const& decl_array = std::get<ast::types::Array>(const_decl.type);

        for (auto const& dim : decl_array.dims) {
          if (!dim)
            throw err::Unsupported{.message =
                                       "Specify the array's indexing set"};

          args.push_back(*dim);
        }
      }
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

    if (!min.isFinite() || !max.isFinite())
      throw err::Unsupported{.message = "Set literal with infinite bound."};

    return ast::BinOp{
        .kind = ast::BinOp::OpKind::DOTDOT,
        .lhs = ast::ptr(ast::LiteralInt{min.toInt()}),
        .rhs = ast::ptr(ast::LiteralInt{max.toInt()}),
        .expr_type = ast::types::IntSet{},
        .is_var = false,
    };
  }

  return ast::LiteralSet{
      .value = set_lit->v() | std::views::transform([&](auto& set_expr) {
                 return map_ptr(set_expr);
               }) |
               std::ranges::to<std::vector>(),
      .expr_type = map(set_lit->type()),
      .is_var = set_lit->type().isvar(),
  };
}

auto Transformer::map(MiniZinc::Comprehension* comp) -> ast::Comprehension {
  std::vector<ast::Generator> generators;

  auto scope = stack.scope();

  for (auto const i : std::views::iota(0u, comp->numberOfGenerators())) {

    if (!comp->in(i) && !comp->where(i))
      throw err::Unsupported{.message = "Comprehension has null generator."};

    if (comp->in(i) != nullptr) {
      ast::ExprHandle const in_expr = map_ptr(comp->in(i));

      for (auto const j : std::views::iota(0u, comp->numberOfDecls(i))) {
        ast::VarDecl decl_expr = handle_const_decl(comp->decl(i, j));
        if (!std::holds_alternative<ast::DeclConst>(decl_expr))
          throw err::Unsupported{.message =
                                     "Comprehension containts variable."};

        scope.add(decl_expr);

        generators.push_back(ast::Iterator{
            .variable = std::get<ast::DeclConst>(std::move(decl_expr)),
            .in = in_expr,
        });
      }
    }

    if (comp->where(i) != nullptr)
      generators.push_back(map_ptr(comp->where(i)));
  }

  return ast::Comprehension{
      .body = map_ptr(comp->e()),
      .generators = std::move(generators),
      .expr_type = map(comp->type()),
      .is_var = comp->type().isvar(),
  };
}

auto Transformer::map(MiniZinc::Call* call) -> ast::Expr {
  if (!call->id().c_str() || call->id() == "")
    throw err::Unsupported{.message = "Function call with null function id"};

  if (call->id() == "assert" &&
      MiniZinc::Expression::loc(call).filename().endsWith(
          "stdlib_language.mzn"))
    throw Ignore{};

  auto const old_id = call->id().substr();
  std::vector old_args(call->args().begin(), call->args().end());
  auto const id = reformat_id(old_id);

  auto types = call->args() | std::views::transform([](auto& expr) {
                 return MiniZinc::Expression::type(expr);
               }) |
               std::ranges::to<std::vector<MiniZinc::Type>>();

  MiniZinc::FunctionI* function_item = nullptr;
  if (is_any_type_opt(types)) {
    std::vector new_types = deopt_types(types);
    function_item = model.matchFn(env, call->id(), new_types, true);

    if (function_item)
      save(function_item);
  }

  if (!function_item) {
    function_item = model.matchFn(env, call->id(), types, true);
    save(function_item);
  }

  auto const result = ast::Call{
      .id = id,
      .args = call->args() |
              std::views::transform([&](auto& arg) { return map_ptr(arg); }) |
              std::ranges::to<std::vector>(),
      .expr_type = map(function_item->ti()),
      .is_var = function_item->ti()->type().isvar(),
  };

  types.push_back(MiniZinc::Type::varbool());

  MiniZinc::GCLock _;
  auto const function_item_reif =
      model.matchReification(env, call->id(), types, true, true);
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

  return ast::ArrayAccess{
      .arr = ast::ptr(std::move(arr)),
      .indexes = std::move(indexes),
      .expr_type = map(array_access->type()),
      .is_var = array_access->type().isvar(),
      .is_index_var_type = is_index_var_type,
  };
}

void Transformer::save(MiniZinc::FunctionI* function) {
  if (function->e() == nullptr)
    return;

  auto const id = reformat_id(function->id().substr());

  if (functions.find(id) != functions.end())
    return;

  functions.emplace(id, ast::Function{});

  auto scope = stack.scope();

  std::vector<ast::IdExpr> params;
  for (auto const ix : std::views::iota(0u, function->paramCount())) {
    std::optional const var = map(function->param(ix), false, false, true);
    if (!var)
      throw err::Unsupported{
          .message = "Parameter of a function should be a valid var decl"};

    scope.add(*var);

    params.push_back(ast::IdExpr::from_var(*var));
  }

  auto& fn = functions[id];
  fn.id = id;
  fn.params = params;
  fn.body = map_ptr(function->e());
}

auto Transformer::map(MiniZinc::Expression* expr) -> ast::Expr try {
  switch (MiniZinc::Expression::eid(expr)) {
  case MiniZinc::IntLit::eid: {
    auto const* int_lit = MiniZinc::Expression::cast<MiniZinc::IntLit>(expr);
    auto const value = MiniZinc::IntLit::v(int_lit);

    if (!value.isFinite())
      throw err::Unsupported{.message = "Infinite integer"};

    return ast::LiteralInt{value.toInt()};
  }
  case MiniZinc::Id::eid: {
    auto* id = MiniZinc::Expression::cast<MiniZinc::Id>(expr);
    if (!id->v().c_str() || id->v() == "")
      throw err::Unsupported{.message = "Id with null string"};

    std::string id_str = id->v().substr();
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

    if (!value.isFinite())
      throw err::Unsupported{.message = "Infinite float"};

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
                                  ? string_lit->v().substr()
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
    std::println("E_ANON");
    assert(false);
  case MiniZinc::FieldAccess::eid:
    std::println("E_FIELDACCESS");
    assert(false);
  case MiniZinc::TypeInst::eid:
    std::println("E_TI");
    assert(false);
  case MiniZinc::TIId::eid:
    std::println("E_TIID");
    assert(false);
  }
} catch (err::Unsupported& e) {
  e.add_location(make_location(MiniZinc::Expression::loc(expr)));
  throw;
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
    }
  };

  ast::Expr lhs = map(bin_op->lhs());
  ast::Expr rhs = map(bin_op->rhs());

  return ast::BinOp{
      .kind = match_kind(bin_op->op()),
      .lhs = ast::ptr(std::move(lhs)),
      .rhs = ast::ptr(std::move(rhs)),
      .expr_type = map(bin_op->type()),
      .is_var = bin_op->type().isvar(),
  };
}

auto Transformer::map_ptr(MiniZinc::Expression* e) -> ast::ExprHandle {
  return ast::ptr(map(e));
}

auto Transformer::map_opt_ptr(MiniZinc::Expression* e)
    -> std::optional<ast::ExprHandle> {
  if (!e)
    return std::nullopt;
  return map_ptr(e);
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

  return ast::UnaryOp{
      .kind = match_kind(un_op->op()),
      .expr = ast::ptr(std::move(expr)),
      .expr_type = map(un_op->type()),
      .is_var = un_op->type().isvar(),
  };
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

  return ast::IfThenElse{
      .if_then = std::move(if_then),
      .else_expr = std::move(else_expr),
      .expr_type = map(ite->type()),
      .is_var = ite->type().isvar(),
  };
}

auto Transformer::map(MiniZinc::Let* let) -> ast::Expr {
  let_in_ctr++;
  std::string id = std::format("let_in_{}", let_in_ctr);

  auto scope = stack.scope();

  auto is_var_decl = [](auto& expr) {
    return MiniZinc::Expression::eid(expr) == MiniZinc::VarDecl::eid;
  };

  std::vector<ast::VarDecl> args =
      let->let() | std::views::filter(is_var_decl) |
      std::views::transform([&](auto& expr) {
        auto var = map(MiniZinc::Expression::cast<MiniZinc::VarDecl>(expr),
                       false, false, false);
        scope.add(var);

        return var;
      }) |
      std::ranges::to<std::vector>();

  std::vector<ast::ExprHandle> constraints =
      let->let() | std::views::filter(std::not_fn(is_var_decl)) |
      std::views::transform([&](auto& expr) { return map_ptr(expr); }) |
      std::ranges::to<std::vector>();

  ast::Expr function_body = map(let->in());
  ast::Type expr_type = utils::expr_type(function_body);
  bool const is_var = utils::is_expr_var(function_body);

  return ast::LetIn{
      .id = id,
      .declarations = std::move(args),
      .constraints = std::move(constraints),
      .in_expr = ast::ptr(std::move(function_body)),
      .expr_type = std::move(expr_type),
      .is_var = is_var,
  };
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
}

auto Transformer::find_objective_expr() -> ast::Expr {
  for (auto& var_decl_item : model.vardecls()) { // TODO: add const version
    auto var_decl =
        MiniZinc::Expression::cast<MiniZinc::VarDecl>(var_decl_item.e());
    if (var_decl->id()->str() != "_objective")
      continue;

    if (!var_decl->e())
      throw err::Unsupported{.message = "Objective expression is null"};

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
