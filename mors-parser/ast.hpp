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
struct UnaryOp;
struct Call;
struct Comprehension;
struct ArrayAccess;
struct IfThenElse;
struct LetIn;
using Expr = std::variant<LiteralBool, LiteralInt, LiteralFloat, LiteralString,
                          LiteralArray, LiteralSet, IdExpr, BinOp, UnaryOp,
                          Call, Comprehension, ArrayAccess, IfThenElse, LetIn>;
using ExprHandle = std::shared_ptr<Expr>;

namespace types {
struct Array {
  std::vector<std::optional<ExprHandle>> dims;
};
} // namespace types

struct LiteralBool {
  bool value;

  Type expr_type = types::Bool{};
  bool is_var = false;
};

struct LiteralInt {
  long long value;

  Type expr_type = types::Int{};
  bool is_var = false;
};

struct LiteralString {
  std::string value;

  Type expr_type = types::String{};
  bool is_var = false;
};

// TODO: floats not supported for now
struct LiteralFloat {
  double value;

  Type expr_type = types::Float{};
  bool is_var = false;
};

struct LiteralArray {
  std::vector<ExprHandle> value;

  Type expr_type;
  bool is_var;
};

struct LiteralSet {
  std::vector<ExprHandle> value;

  Type expr_type;
  bool is_var;
};

struct DeclVariable;
struct DeclConst;
using VarDecl = std::variant<DeclVariable, DeclConst>;
struct IdExpr {
  std::string id;

  bool is_global;
  bool is_var;

  Type expr_type;

  static auto from_var(VarDecl const& var) -> IdExpr;
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
    OR,
    IMPL,
    IN,
    EQUIV
    // BOT_DIV,
    // BOT_POW,
    // BOT_SUBSET,
    // BOT_SUPERSET,
    // BOT_UNION,
    // BOT_DIFF,
    // BOT_SYMDIFF,
    // BOT_INTERSECT,
    // BOT_RIMPL,
    // BOT_XOR,
  };
  OpKind kind;

  ExprHandle lhs, rhs;

  Type expr_type;
  bool is_var;
};

struct UnaryOp {
  enum class OpKind : uint8_t { NOT, PLUS, MINUS };
  OpKind kind;

  ExprHandle expr;

  Type expr_type;
  bool is_var;
};

struct Call {
  std::string id;

  std::vector<ExprHandle> args;

  Type expr_type;
  bool is_var;
};

struct ArrayAccess {
  ExprHandle arr;

  std::vector<ExprHandle> indexes;

  Type expr_type;
  bool is_var;

  bool is_index_var_type = false;
};

struct IfThenElse {
  std::vector<std::pair<ExprHandle, ExprHandle>> if_then;

  std::optional<ExprHandle> else_expr;

  Type expr_type;
  bool is_var;
};

struct DeclVariable {
  std::string id;

  Type var_type;

  bool is_global = false;

  std::optional<ExprHandle> domain;
  std::optional<ExprHandle> value;
};

struct DeclConst {
  std::string id;
  Type type;

  bool is_global = false;

  std::optional<ExprHandle> value;
};

struct LetIn {
  std::string id;

  std::vector<VarDecl> declarations;
  std::vector<ExprHandle> constraints;

  Type expr_type;
  bool is_var;
};

using Filter = ExprHandle;

struct Iterator {
  DeclConst variable;

  ExprHandle in;
};

using Generator = std::variant<Iterator, Filter>;

struct Comprehension {
  ExprHandle body;

  std::vector<Generator> generators;

  Type expr_type;
  bool is_var;
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
  std::vector<IdExpr> params;

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

  FunctionMap functions;
};

template <typename T>
auto ptr(T && t) -> ExprHandle {
    return std::make_shared<Expr>(std::forward<T>(t));
}


auto ptr(Expr&& t) -> ExprHandle;

} // namespace parser::ast
