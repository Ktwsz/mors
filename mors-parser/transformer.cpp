#include "transformer.hpp"
#include "ast.hpp"

#include <fmt/base.h>
#include <minizinc/type.hh>

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

auto Transformer::handle_domain(MiniZinc::BinOp* bin_op) -> ast::Domain {
  switch (bin_op->op()) {
  case MiniZinc::BOT_DOTDOT: {
    return ast::Domain{.lower = map_expr(bin_op->lhs()),
                       .upper = map_expr(bin_op->rhs())};
  }
  default:
    assert(false);
  }
}

auto Transformer::handle_domain(MiniZinc::Expression* expr) -> ast::Domain {
  switch (MiniZinc::Expression::eid(expr)) {
  case MiniZinc::Expression::E_BINOP: {
    auto* bin_op = MiniZinc::Expression::cast<MiniZinc::BinOp>(expr);
    return handle_domain(bin_op);
  }
  default:
    assert(false);
  }
}

auto Transformer::handle_const_decl(MiniZinc::VarDecl* var_decl)
    -> ast::ASTNode {
  return ast::DeclConst{.id = std::string{var_decl->id()->v().c_str()},
                        .type = resolve_expr_type(var_decl->e()),
                        .value = map_expr(var_decl->e())};
}

auto Transformer::handle_var_decl(MiniZinc::VarDecl* var_decl) -> ast::ASTNode {
  assert(var_decl->ti()->domain() != nullptr);
  return ast::DeclVariable{.id = std::string{var_decl->id()->v().c_str()},
                           .domain = handle_domain(var_decl->ti()->domain())};
}

auto Transformer::map_vardecl(MiniZinc::VarDecl* var_decl)
    -> std::optional<ast::ASTNode> {
  if (!var_decl->item()->loc().filename().endsWith(input_model_path))
    return std::nullopt;

  if (var_decl->e() != nullptr)
    return handle_const_decl(var_decl);

  return handle_var_decl(var_decl);
}

auto Transformer::map_expr(MiniZinc::Expression* expr) -> ast::Expr {
  switch (MiniZinc::Expression::eid(expr)) {
  case MiniZinc::Expression::E_INTLIT: {
    auto const* int_lit = MiniZinc::Expression::cast<MiniZinc::IntLit>(expr);
    auto const value = MiniZinc::IntLit::v(int_lit);

    assert(value.isFinite());

    return ast::LiteralInt{value.toInt()};
  }
  case MiniZinc::Expression::E_ID: {
    auto* id = MiniZinc::Expression::cast<MiniZinc::Id>(expr);
    return ast::IdExpr{std::string{id->v().c_str()}};
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
  // case MiniZinc::Expression::E_BINOP: {
  //   auto* bin_op = MiniZinc::Expression::cast<MiniZinc::BinOp>(expr);
  //   print_bin_op(bin_op, indent);
  //   break;
  // }
  // case MiniZinc::Expression::E_UNOP:
  //   fmt::println("E_UNOP");
  //   break;
  // case MiniZinc::Expression::E_CALL: {
  //   auto* call = MiniZinc::Expression::cast<MiniZinc::Call>(expr);
  //   print_fn_call(call, indent);
  //   break;
  // }
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

} // namespace parser
