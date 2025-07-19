#include "lib.hpp"
#include "ast_printer.hpp"
#include "transformer.hpp"

#include <iostream>
#include <ranges>
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
constexpr std::string_view verbose = "-v"sv;
} // namespace flags

auto main(ParserOpts const& opts) -> std::expected<ast::Tree, err::Error> {
  if (opts.verbose)
    fmt::println("std path: {}", opts.stdlib_dir);

  if (opts.verbose)
    fmt::println("OR-Tools path: {}", opts.ortools_include_dir);

  std::ostringstream flattener_os, flattener_log;
  auto flt = MiniZinc::Flattener{flattener_os, flattener_log, opts.stdlib_dir};
  flt.setFlagVerbose(opts.verbose);

  std::vector const flattener_args{
      opts.model_path, std::string{flags::instance_check_only},
      std::string{flags::include}, opts.ortools_include_dir};

  for (int i = 0; static_cast<size_t>(i) < flattener_args.size(); i++) {
    if (auto const ok = flt.processOption(i, flattener_args); !ok) {
      return std::unexpected{
          err::InvalidFlag{.os = std::move(flattener_os),
                           .log = std::move(flattener_log)}
      };
    }
  }

  for (int i = 0; static_cast<size_t>(i) < opts.infiles.size(); i++) {
    if (auto const ok = flt.processOption(i, opts.infiles); !ok) {
      return std::unexpected{
          err::InvalidFlag{.os = std::move(flattener_os),
                           .log = std::move(flattener_log)}
      };
    }
  }

  if (auto const warnings_view = flattener_log.view(); !warnings_view.empty())
    fmt::println("MiniZinc Parser returned warnings:\n{}", warnings_view);

  try {
    flt.flatten("", "stdin");
  } catch (MiniZinc::Exception const& e) {
    return std::unexpected{
        err::MznParsingError{
                             .os = std::move(flattener_os),
                             .log = std::move(flattener_log),
                             .msg = fmt::format("parsing failed:\n{}\n", e.msg())}
    };
  }

  if (auto const warnings_view = flattener_log.view(); !warnings_view.empty())
    fmt::println("MiniZinc Parser returned warnings:\n{}", warnings_view);

  auto& model = *flt.getEnv()->model();

  if (opts.print_ast) {
    PrintModelVisitor vis{model, flt.getEnv()->envi(), opts.model_path};

    std::cout << "--- VAR DECLS ---" << std::endl;
    for (auto& var_decl : model.vardecls()) {
      vis.print_var_decl(var_decl.e(), 0);
    }

    // fmt::println("--- AST ---");
    //
    // MiniZinc::iter_items<PrintModelVisitor>(vis, &model);
    //
    // fmt::println("-----------");
    // TODO: separate ast printing from this function
    return std::unexpected{err::MznParsingError{}};
  }

  Transformer transformer{.model = model,
                          .env = flt.getEnv()->envi(),
                          .input_model_path = opts.model_path};

  ast::Tree tree;
  for (auto& var_decl : model.vardecls()) {
    if (auto const decl = transformer.map_vardecl(var_decl.e()); decl)
      tree.decls.push_back(*decl);
  }
  // auto const decls = model.vardecls() |
  //                    std::views::transform([&](MiniZinc::VarDeclI& var_decl)
  //                    {
  //                      return transformer.map_vardecl(var_decl.e());
  //                    }) |
  //                    std::ranges::to<std::vector<ast::ASTNode>>;

  return tree;
}

} // namespace parser
