#include "ast.hpp"

#include "utils.hpp"

#include <utility>
#include <variant>

namespace parser::ast {

auto IdExpr::from_var(VarDecl const& var) -> IdExpr {
  return IdExpr{.id = utils::var_id(var),
                .is_global = utils::is_var_global(var),
                .is_var = utils::is_var_var(var),
                .expr_type = utils::var_type(var)};
}

void Tree::make_output() {
  std::vector<ExprHandle> exprs;
  for (VarDecl const& var_decl : decls) {
    if (!std::holds_alternative<DeclVariable>(var_decl))
      continue;

    auto const& var = std::get<DeclVariable>(var_decl);

    auto bin_op = BinOp{
        .kind = BinOp::OpKind::PLUSPLUS,
        .lhs = ptr(LiteralString{var.id + "="}),
        .rhs = ptr(Call{
            .id = "show",
            .args = {ptr(IdExpr::from_var(var))},
            .expr_type = types::String{},
            .is_var = false,
        }),
        .expr_type = types::String{},
        .is_var = false,
    };

    exprs.push_back(ptr(std::move(bin_op)));
    exprs.push_back(ptr(LiteralString{"\n"}));
  }

  output = ptr(ast::LiteralArray{
      .value = std::move(exprs),
      .expr_type =
          types::Array{
                       .dims = {},
                       .inner_type = std::make_shared<Type>(types::String{}),
                       },
      .is_var = false,
  });
}

} // namespace parser::ast
