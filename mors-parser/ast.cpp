#include "ast.hpp"

#include "utils.hpp"

#include <utility>

namespace parser::ast {

auto IdExpr::from_var(std::string_view const id, VarDecl const& var) -> IdExpr {
  return IdExpr{.id = std::string{id},
                .is_global = utils::is_var_global(var),
                .is_var = utils::is_var_var(var),
                .expr_type = utils::var_type(var)};
}

template <typename T>
auto ptr(T && t) -> ExprHandle {
    return std::make_shared<Expr>(std::forward<T>(t));
}

auto ptr(Expr && t) -> ExprHandle {
    return std::make_shared<Expr>(std::forward<Expr>(t));
}

} // namespace parser::ast
