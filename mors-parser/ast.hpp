#pragma once

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace parser::ast {

struct LiteralInt {
  long long value;
};

struct IdExpr {
  std::string id;
};

using Expr = std::variant<LiteralInt, IdExpr>;

struct Domain {
  Expr lower, upper;
};

namespace types {
struct Int {};
} // namespace types

using Type = std::variant<types::Int>;

struct DeclVariable {
  std::string id;

  std::optional<Domain> domain;
};

struct DeclConst {
  std::string id;
  Type type;

  Expr value;
};

using ASTNode = std::variant<DeclVariable, DeclConst>;

struct Tree {
  std::vector<ASTNode> decls;
};
// case MiniZinc::Expression::E_INTLIT
// case MiniZinc::Expression::E_FLOATLIT:
// case MiniZinc::Expression::E_SETLIT:
// case MiniZinc::Expression::E_BOOLLIT:
// case MiniZinc::Expression::E_STRINGLIT:
// case MiniZinc::Expression::E_ID:
// case MiniZinc::Expression::E_ANON:
// case MiniZinc::Expression::E_ARRAYLIT:
// case MiniZinc::Expression::E_ARRAYACCESS:
// case MiniZinc::Expression::E_FIELDACCESS:
// case MiniZinc::Expression::E_COMP:
// case MiniZinc::Expression::E_ITE: {
// case MiniZinc::Expression::E_BINOP: {
// case MiniZinc::Expression::E_UNOP:
// case MiniZinc::Expression::E_CALL: {
// case MiniZinc::Expression::E_VARDECL: {
// case MiniZinc::Expression::E_LET: {
// case MiniZinc::Expression::E_TI:
// case MiniZinc::Expression::E_TIID:

} // namespace parser::ast
