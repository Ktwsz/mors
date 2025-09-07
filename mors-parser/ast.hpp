#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace parser::ast {
namespace types {
struct Unspecified {};
struct Int {};
struct Float {}; // TODO: floats not supported for now
struct Bool {};
struct String {};

template <typename T> struct Set {};

using IntSet = Set<Int>;
using FloatSet = Set<Float>;
using BoolSet = Set<Bool>;

using UnspecifiedSet = Set<Unspecified>;

struct Array;

} // namespace types

using Type = std::variant<types::Int, types::Float, types::Bool, types::String,
                          types::IntSet, types::FloatSet, types::BoolSet,
                          types::UnspecifiedSet, types::Array>;

struct LiteralBool;
struct LiteralInt;
struct LiteralFloat;
struct LiteralString;
struct LiteralArray;
struct LiteralSet;
struct IdExpr;
struct BinOp;
struct Call;
struct Comprehension;
struct ArrayAccess;
struct IfThenElse;
using Expr = std::variant<LiteralBool, LiteralInt, LiteralFloat, LiteralString,
                          LiteralArray, LiteralSet, IdExpr, BinOp, Call,
                          Comprehension, ArrayAccess, IfThenElse>;
using ExprHandle = std::shared_ptr<Expr>;

namespace types {
struct Array {
  std::vector<std::optional<ExprHandle>> dims;
};
} // namespace types

struct LiteralBool {
  bool value;

  Type expr_type = types::Bool{};
};

struct LiteralInt {
  long long value;

  Type expr_type = types::Int{};
};

struct LiteralString {
  std::string value;

  Type expr_type = types::String{};
};

// TODO: floats not supported for now
struct LiteralFloat {
  double value;

  Type expr_type = types::Float{};
};

struct LiteralArray {
  std::vector<ExprHandle> value;

  Type expr_type = types::Array{};
};

struct LiteralSet {
  std::vector<ExprHandle> value;

  Type expr_type = types::UnspecifiedSet{};
};

struct IdExpr {
  std::string id;

  bool is_global;
  bool is_var;

  Type expr_type;
};

struct BinOp {
  enum class OpKind : uint8_t {
    PLUS,
    MINUS,
    MULT,
    IDIV,
    MOD,
    DOTDOT,
    EQ,
    NQ,
    GQ,
    GR,
    LE,
    LQ,
    PLUSPLUS,
    AND,
    OR
  };
  OpKind kind;

  ExprHandle lhs, rhs;

  Type expr_type;
};

struct Call {
  std::string id;

  std::vector<ExprHandle> args;

  Type expr_type;
};

struct ArrayAccess {
  ExprHandle arr;

  std::vector<ExprHandle> indexes;

  Type expr_type = types::Int{};
};

struct IfThenElse {
  std::vector<std::pair<ExprHandle, ExprHandle>> if_then;

  std::optional<ExprHandle> else_expr;

  Type expr_type;
};

struct DeclVariable {
  std::string id;

  Type var_type;

  bool is_global = false;

  std::optional<ExprHandle> domain;
};

struct DeclConst {
  std::string id;
  Type type;

  bool is_global = false;

  std::optional<ExprHandle> value;
};

using VarDecl = std::variant<DeclVariable, DeclConst>;

using Filter = ExprHandle;

struct Iterator {
  DeclConst variable;

  ExprHandle in;
};

using Generator = std::variant<Iterator, Filter>;

struct Comprehension {
  ExprHandle body;

  std::vector<Generator> generators;

  Type expr_type = types::Array{};
};

namespace solve_type {

struct Sat {};
struct Min {
  ExprHandle expr;
};
struct Max {
  ExprHandle expr;
};

} // namespace solve_type

struct Function {
  std::string id;
  std::vector<std::string> params;

  ExprHandle body;
};

using FunctionMap = std::map<std::string, Function>;
using VariableMap = std::map<std::string, std::vector<VarDecl>>;

using SolveType =
    std::variant<solve_type::Sat, solve_type::Max, solve_type::Min>;

struct Tree {
  std::vector<VarDecl> decls;
  std::vector<ExprHandle> constraints;

  SolveType solve_type;

  ExprHandle output;

  VariableMap variable_map;
  FunctionMap functions;
};

} // namespace parser::ast
