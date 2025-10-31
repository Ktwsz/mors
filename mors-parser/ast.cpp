#include "ast.hpp"

#include "utils.hpp"

#include <utility>

namespace parser::ast {

auto IdExpr::from_var(VarDecl const& var) -> IdExpr {
  return IdExpr{.id = std::string{utils::var_id(var)},
                .is_global = utils::is_var_global(var),
                .is_var = utils::is_var_var(var),
                .expr_type = utils::var_type(var)};
}

auto ptr(Expr && t) -> ExprHandle {
    return std::make_shared<Expr>(std::forward<Expr>(t));
}

} // namespace parser::ast
