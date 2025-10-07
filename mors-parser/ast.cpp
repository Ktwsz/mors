#include "ast.hpp"

#include "utils.hpp"

namespace parser::ast {

auto IdExpr::from_var(std::string_view const id, VarDecl const& var) -> IdExpr {
  return IdExpr{.id = std::string{id},
                .is_global = utils::is_var_global(var),
                .is_var = utils::is_var_var(var),
                .expr_type = utils::var_type(var)};
}

} // namespace parser::ast
