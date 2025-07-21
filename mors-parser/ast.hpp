#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace parser::ast {

struct LiteralInt;
struct IdExpr;
struct BinOp;
using Expr = std::variant<LiteralInt, IdExpr, BinOp>;
using ExprHandle = std::shared_ptr<Expr>;

struct LiteralInt {
  long long value;
};

struct IdExpr {
  std::string id;
};

struct BinOp {
  enum class OpKind : uint8_t { DOTDOT, NQ };
  OpKind kind;

  ExprHandle lhs, rhs;
};

namespace types {
struct Int {};
} // namespace types

using Type = std::variant<types::Int>;

struct DeclVariable {
  std::string id;

  std::optional<ExprHandle> domain;
};

struct DeclConst {
  std::string id;
  Type type;

  ExprHandle value;
};

using VarDecl = std::variant<DeclVariable, DeclConst>;

struct Tree {
  std::vector<VarDecl> decls;
  std::vector<ExprHandle> constraints;
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
