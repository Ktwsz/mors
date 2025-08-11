#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace parser::ast {

struct LiteralInt;
struct LiteralFloat;
struct LiteralString;
struct LiteralArray;
struct IdExpr;
struct BinOp;
struct Call;
using Expr = std::variant<LiteralInt, LiteralFloat, LiteralString, LiteralArray,
                          IdExpr, BinOp, Call>;
using ExprHandle = std::shared_ptr<Expr>;

struct LiteralInt {
  long long value;
};

struct LiteralString {
  std::string value;
};

struct LiteralFloat {
  double value;
};

struct LiteralArray {
  std::vector<ExprHandle> value;
};

struct IdExpr {
  std::string id;
};

struct BinOp {
  enum class OpKind : uint8_t {
    PLUS,
    MINUS,
    MULT,
    DIV,
    DOTDOT,
    EQ,
    NQ,
    PLUSPLUS
  };
  OpKind kind;

  ExprHandle lhs, rhs;
};

struct Call {
  std::string id;

  std::vector<ExprHandle> args;
};

namespace types {
struct Int {};
struct Float {};
struct Bool {};

template <typename T> struct Set {};

struct Array;

} // namespace types

using Type = std::variant<types::Int, types::Float, types::Bool,
                          types::Set<types::Int>, types::Set<types::Float>,
                          types::Set<types::Bool>, types::Array>;
using TypeHandle = std::shared_ptr<Type>;


namespace types {
struct Array {
  std::vector<TypeHandle> dims;
};
} // namespace types

struct DeclVariable {
  std::string id;

  Type var_type;

  std::optional<ExprHandle> domain;
};

struct DeclConst {
  std::string id;
  Type type;

  ExprHandle value;
};

using VarDecl = std::variant<DeclVariable, DeclConst>;

enum struct SolveType { SAT, MIN, MAX };

struct Tree {
  std::vector<VarDecl> decls;
  std::vector<ExprHandle> constraints;

  SolveType solve_type;

  ExprHandle output;
};
// case MiniZinc::Expression::E_SETLIT:
// case MiniZinc::Expression::E_ANON:
// case MiniZinc::Expression::E_ARRAYACCESS:
// case MiniZinc::Expression::E_FIELDACCESS:
// case MiniZinc::Expression::E_COMP:
// case MiniZinc::Expression::E_ITE:
// case MiniZinc::Expression::E_UNOP:
// case MiniZinc::Expression::E_CALL:
// case MiniZinc::Expression::E_LET:
// case MiniZinc::Expression::E_TI:
// case MiniZinc::Expression::E_TIID:

} // namespace parser::ast
