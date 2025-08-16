#include "transformer.hpp"
#include "ast.hpp"

#include <fmt/base.h>
#include <minizinc/type.hh>

#include <optional>
#include <ranges>
#include <utility>
#include <variant>

namespace parser {
namespace {
template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};

auto resolve_base_type(MiniZinc::Type::BaseType base_type) -> ast::Type {
  // enum BaseType {
  //   BT_STRING,
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
  default:
    assert(false);
  }
}
} // namespace

auto Transformer::map(MiniZinc::TypeInst* type_inst) -> ast::Type {
  if (type_inst->isarray()) {
    ast::types::Array arr{};

    for (auto inner : type_inst->ranges()) {
      assert(inner->domain() != nullptr && "prayge");
      arr.dims.push_back(*map(inner->domain()));
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

auto Transformer::map(MiniZinc::VarDecl* var_decl)
    -> std::optional<ast::VarDecl> {
  if (!var_decl->item()->loc().filename().endsWith(input_model_path))
    return std::nullopt;

  if (MiniZinc::Expression::type(var_decl->ti()).isPar())
    return handle_const_decl(var_decl);

  return handle_var_decl(var_decl);
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
        .rhs = std::make_shared<ast::Expr>(ast::LiteralInt{max.toInt()})});
  }

  auto ast_set = std::make_shared<ast::Expr>(ast::LiteralSet{});
  for (auto& set_expr : set_lit->v()) {
    auto ast_expr = map(set_expr);
    assert(ast_expr && "Set Literal: nullopt item");

    std::get<ast::LiteralArray>(*ast_set).value.push_back(*ast_expr);
  }

  return ast_set;
}

auto Transformer::map(MiniZinc::Comprehension* comp) -> ast::Comprehension {
  auto body = map(comp->e());
  assert(body);

  std::vector<ast::Generator> generators;

  for (auto const i :
       std::views::iota(0u, comp->numberOfGenerators()) | std::views::reverse) {

    if (comp->in(i) == nullptr)
      continue;

    auto const in_expr = map(comp->in(i));
    assert(in_expr);

    for (auto const j :
         std::views::iota(0u, comp->numberOfDecls(i)) | std::views::reverse) {
      auto decl_expr = handle_const_decl(comp->decl(i, j));
      assert(std::holds_alternative<ast::DeclConst>(decl_expr));

      generators.push_back(ast::Generator{
          .variable = std::get<ast::DeclConst>(decl_expr), .in = *in_expr});
    }
  }

  return ast::Comprehension{.body = std::move(*body),
                            .generators = std::move(generators)};
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
    return std::make_shared<ast::Expr>(
        ast::IdExpr{std::string{id->v().c_str()}});
  }
  case MiniZinc::Expression::E_BINOP: {
    auto* bin_op = MiniZinc::Expression::cast<MiniZinc::BinOp>(expr);
    return map(bin_op);
  }
  case MiniZinc::Expression::E_CALL: {
    auto* call = MiniZinc::Expression::cast<MiniZinc::Call>(expr);

    assert(call->id().c_str() != nullptr && "Function call: null function id");
    auto const id = std::string{call->id().c_str()};
    assert(!id.empty() && "Function call: empty function id");

    if (id == "assert") {
      fmt::println("skipping assert calls for now");
      return std::nullopt;
    }

    // TODO: remember to copy function declaration as well

    auto ast_call =
        std::make_shared<ast::Expr>(ast::Call{.id = id, .args = {}});
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
  // case MiniZinc::Expression::E_ANON:
  //   fmt::println("E_ANON");
  //   break;
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
  // case MiniZinc::Expression::E_FIELDACCESS:
  //   fmt::println("E_FIELDACCESS");
  //   break;
  // case MiniZinc::Expression::E_ITE: {
  //   auto* ite = MiniZinc::Expression::cast<MiniZinc::ITE>(expr);
  //   print_ite(ite, indent);
  //   break;
  // }
  // case MiniZinc::Expression::E_UNOP:
  //   fmt::println("E_UNOP");
  //   break;
  // case MiniZinc::Expression::E_VARDECL: {
  //   auto* varDecl = MiniZinc::Expression::cast<MiniZinc::VarDecl>(expr);
  //   print_var_decl(varDecl, indent);
  //   break;
  // }
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
    default:
      assert(false);
    }
  };

  auto lhs = map(bin_op->lhs());
  auto rhs = map(bin_op->rhs());
  if (!lhs || !rhs)
    assert(false);

  return std::make_shared<ast::Expr>(
      ast::BinOp{.kind = match_kind(bin_op->op()),
                 .lhs = std::move(*lhs),
                 .rhs = std::move(*rhs)});
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
