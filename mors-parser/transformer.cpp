#include "transformer.hpp"
#include "ast.hpp"

#include <fmt/base.h>
#include <minizinc/type.hh>
#include <utility>

namespace parser {
namespace {
auto resolve_expr_type(MiniZinc::Expression* expr) -> ast::Type {
  // enum BaseType {
  //   BT_BOOL,
  //   BT_INT,
  //   BT_FLOAT,
  //   BT_STRING,
  //   BT_ANN,
  //   BT_TUPLE,
  //   BT_RECORD,
  //   BT_TOP,
  //   BT_BOT,
  //   BT_UNKNOWN
  // };
  switch (MiniZinc::Expression::type(expr).bt()) {
  case MiniZinc::Type::BT_INT: {
    return ast::types::Int{};
  }
  default:
    assert(false);
  }
}
} // namespace

auto Transformer::handle_const_decl(MiniZinc::VarDecl* var_decl)
    -> ast::VarDecl {
  auto value = map(var_decl->e());
  if (!value)
    assert(false);

  return ast::DeclConst{.id = std::string{var_decl->id()->v().c_str()},
                        .type = resolve_expr_type(var_decl->e()),
                        .value = *value};
}

auto Transformer::handle_var_decl(MiniZinc::VarDecl* var_decl) -> ast::VarDecl {
  assert(var_decl->ti()->domain() != nullptr);
  return ast::DeclVariable{.id = std::string{var_decl->id()->v().c_str()},
                           .domain = map(var_decl->ti()->domain())};
}

auto Transformer::map(MiniZinc::VarDecl* var_decl)
    -> std::optional<ast::VarDecl> {
  if (!var_decl->item()->loc().filename().endsWith(input_model_path))
    return std::nullopt;

  if (var_decl->e() != nullptr)
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
    return std::make_shared<ast::Expr>(
        ast::IdExpr{std::string{id->v().c_str()}});
  }
  case MiniZinc::Expression::E_BINOP: {
    auto* bin_op = MiniZinc::Expression::cast<MiniZinc::BinOp>(expr);
    return map(bin_op);
  }
  case MiniZinc::Expression::E_CALL: {
    fmt::println("skipping assert calls for now");
    return std::nullopt;
    // auto* call = MiniZinc::Expression::cast<MiniZinc::Call>(expr);
    // print_fn_call(call, indent);
    // break;
  }
  // case MiniZinc::Expression::E_FLOATLIT:
  //   fmt::println("E_FLOATLIT");
  //   break;
  // case MiniZinc::Expression::E_SETLIT:
  //   fmt::println("E_SETLIT");
  //   break;
  // case MiniZinc::Expression::E_BOOLLIT:
  //   fmt::println("E_BOOLLIT");
  //   break;
  // case MiniZinc::Expression::E_STRINGLIT:
  //   fmt::println("E_STRINGLIT");
  //   break;
  // case MiniZinc::Expression::E_ANON:
  //   fmt::println("E_ANON");
  //   break;
  // case MiniZinc::Expression::E_ARRAYLIT:
  //   fmt::println("E_ARRAYLIT");
  //   break;
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
    return ast::SolveType::MAX;
  case MiniZinc::SolveI::SolveType::ST_MIN:
    return ast::SolveType::MIN;
  case MiniZinc::SolveI::SolveType::ST_SAT:
    return ast::SolveType::SAT;
  }
}

} // namespace parser
