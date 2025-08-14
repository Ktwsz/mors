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
struct Comprehension;
struct ArrayAccess;
using Expr = std::variant<LiteralInt, LiteralFloat, LiteralString, LiteralArray,
                          IdExpr, BinOp, Call, Comprehension, ArrayAccess>;
using ExprHandle = std::shared_ptr<Expr>;

struct LiteralInt {
  long long value;
};

struct LiteralString {
  std::string value;
};

// TODO: floats not supported for now
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
    IDIV,
    DOTDOT,
    EQ,
    NQ,
    GQ,
    GR,
    LE,
    LQ,
    PLUSPLUS
  };
  OpKind kind;

  ExprHandle lhs, rhs;
};

struct Call {
  std::string id;

  std::vector<ExprHandle> args;
};

struct ArrayAccess {
  ExprHandle arr;

  std::vector<ExprHandle> indexes;
};

namespace types {
struct Int {};
struct Float {}; // TODO: floats not supported for now
struct Bool {};

template <typename T> struct Set {};

using IntSet = Set<Int>;
using FloatSet = Set<Float>;
using BoolSet = Set<Bool>;

struct Array;

} // namespace types

using Type = std::variant<types::Int, types::Float, types::Bool,
                          types::Set<types::Int>, types::Set<types::Float>,
                          types::Set<types::Bool>, types::Array>;
// using TypeHandle = std::shared_ptr<Type>;

namespace types {
struct Array {
  // std::vector<TypeHandle> dims;
  std::vector<ExprHandle> dims;
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

  std::optional<ExprHandle> value;
};

using VarDecl = std::variant<DeclVariable, DeclConst>;

struct Generator {
  DeclConst variable;

  ExprHandle in;
};

struct Comprehension {
  ExprHandle body;

  std::vector<Generator> generators;
};

//TODO - expr to solve
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
