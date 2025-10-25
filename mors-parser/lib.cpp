#include "lib.hpp"
#include "ast_printer.hpp"
#include "minizinc/ast.hh"
#include "transformer.hpp"

#include <sstream>
#include <string_view>

#include <fmt/base.h>
#include <fmt/format.h>
#include <minizinc/flattener.hh>

namespace parser {
namespace flags {
using namespace std::string_view_literals;

constexpr std::string_view instance_check_only = "--instance-check-only"sv;
constexpr std::string_view include = "-I"sv;
} // namespace flags

namespace {

void log_flags(ParserOpts const& opts) {

  if (opts.verbose)
    fmt::println("std path: {}", opts.stdlib_dir);

  if (opts.verbose)
    fmt::println("OR-Tools path: {}", opts.ortools_include_dir);

  if (opts.verbose)
    fmt::println("output file: {}", opts.get_output_file());
}

auto create_flags(ParserOpts const& opts) -> std::vector<std::string> {
  return {opts.model_path, std::string{flags::instance_check_only},
          std::string{flags::include}, opts.ortools_include_dir};
}

auto feed_flags(MiniZinc::Flattener& flt, ParserOpts const& opts,
                std::ostringstream& flattener_os,
                std::ostringstream& flattener_log)
    -> std::optional<err::Error> {
  auto flattener_args = create_flags(opts);

  for (int i = 0; static_cast<size_t>(i) < flattener_args.size(); i++) {
    if (auto const ok = flt.processOption(i, flattener_args); !ok) {
      return err::InvalidFlag{.os = std::move(flattener_os),
                              .log = std::move(flattener_log)};
    }
  }

  for (int i = 0; static_cast<size_t>(i) < opts.infiles.size(); i++) {
    if (auto const ok = flt.processOption(i, opts.infiles); !ok) {
      return err::InvalidFlag{.os = std::move(flattener_os),
                              .log = std::move(flattener_log)};
    }
  }

  return std::nullopt;
}

} // namespace

auto main(ParserOpts const& opts) -> std::expected<ast::Tree, err::Error> {
  log_flags(opts);

  std::ostringstream flattener_os, flattener_log;
  MiniZinc::Flattener flt{flattener_os, flattener_log, opts.stdlib_dir};
  flt.setFlagVerbose(opts.verbose);

  if (auto err = feed_flags(flt, opts, flattener_os, flattener_log); err)
    return std::unexpected{std::move(*err)};

  if (auto const warnings_view = flattener_log.view(); !warnings_view.empty())
    fmt::println("MiniZinc Parser returned warnings:\n{}", warnings_view);

  try {
    flt.flatten("", "stdin");
  } catch (MiniZinc::Exception const& e) {
    std::ostringstream mzn_output{};
    e.print(mzn_output);
    return std::unexpected{
        err::MznParsingError{.os = std::move(flattener_os),
                             .log = std::move(flattener_log),
                             .msg = mzn_output.str()}
    };
  }

  if (auto const warnings_view = flattener_log.view(); !warnings_view.empty())
    fmt::println("MiniZinc Parser returned warnings:\n{}", warnings_view);

  auto& model = *flt.getEnv()->model();

  // TODO: separate ast printing from this function
  if (opts.print_ast) {
    PrintModelVisitor vis{model, flt.getEnv()->envi(), opts.model_path};

    fmt::println("--- VAR DECLS ---");
    for (auto& var_decl : model.vardecls()) {
      vis.print_var_decl(var_decl.e(), 0);
    }

    fmt::println("--- CONSTRAINTS ---");
    for (auto& constraint : model.constraints()) {
      vis.match_expr(constraint.e(), 0);
    }

    fmt::println("--- SOLVE ---");
    vis.print_solve_type(model.solveItem());
    if (model.solveItem()->st() != MiniZinc::SolveI::ST_SAT) {
      for (auto& var_decl : model.vardecls()) {
        if (MiniZinc::Expression::cast<MiniZinc::VarDecl>(var_decl.e())
                ->id()
                ->str() != "_objective")
          continue;

        vis.print_var_decl(var_decl.e(), 4);
      }
    }

    fmt::println("--- OUTPUT ---");
    vis.match_expr(model.outputItem()->e());

    return ast::Tree{};
  }

  ast::Tree tree;
  Transformer transformer{.model = model,
                          .env = flt.getEnv()->envi(),
                          .functions = tree.functions,
                          .opts = opts};

  for (auto& var_decl : model.vardecls()) { // TODO make view compatible
    if (auto decl = transformer.map(var_decl.e(), true, true); decl) {
      tree.decls.push_back(std::move(*decl));
    }
  }

  for (auto& constraint : model.constraints()) { // TODO make view compatible
    tree.constraints.push_back(transformer.map_ptr(constraint.e()));
  }

  tree.solve_type = transformer.map(model.solveItem());

  tree.output = transformer.map_ptr(model.outputItem()->e());

  return tree;
}

} // namespace parser
