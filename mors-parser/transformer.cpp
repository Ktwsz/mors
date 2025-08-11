#include "transformer.hpp"
#include "ast.hpp"

#include <fmt/base.h>
#include <minizinc/type.hh>

#include <optional>
#include <utility>
#include <variant>

namespace parser {
namespace {
template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

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
  auto value = map(var_decl->e());
  if (!value)
    assert(false);

  return ast::DeclConst{.id = std::string{var_decl->id()->v().c_str()},
                        .type = map(var_decl->ti()),
                        .value = *value};
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
  // case MiniZinc::Expression::E_SETLIT:
  //   fmt::println("E_SETLIT");
  //   break;
  // case MiniZinc::Expression::E_BOOLLIT:
  //   fmt::println("E_BOOLLIT");
  //   break;
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
  // case MiniZinc::Expression::E_ARRAYACCESS:
  //   fmt::println("E_ARRAYACCESS");
  //   break;
  // case MiniZinc::Expression::E_FIELDACCESS:
  //   fmt::println("E_FIELDACCESS");
  //   break;
  // case MiniZinc::Expression::E_COMP:
  //   fmt::println("E_COMP");
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
    case MiniZinc::BOT_DIV:
      return ast::BinOp::OpKind::DIV;
    case MiniZinc::BOT_EQ:
      return ast::BinOp::OpKind::EQ;
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
  case MiniZinc::SolveI::SolveType::ST_MAX: // TODO: read expr to maximise
    return ast::SolveType::MAX;
  case MiniZinc::SolveI::SolveType::ST_MIN: // TODO: read expr to minimise
    return ast::SolveType::MIN;
  case MiniZinc::SolveI::SolveType::ST_SAT:
    return ast::SolveType::SAT;
  }
  assert(false);
}

} // namespace parser
