#include "lib.hpp"
#include "ast_printer.hpp"

#include <iostream>
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

auto main(ParserOpts const& opts) -> std::expected<IR::Data, ParsingError> {
  if (opts.verbose)
    fmt::println("std path: {}", opts.stdlib_dir);

  if (opts.verbose)
    fmt::println("OR-Tools path: {}", opts.ortools_include_dir);

  std::stringstream flattener_os, flattener_log;
  auto flt = MiniZinc::Flattener{flattener_os, flattener_log, opts.stdlib_dir};
  flt.setFlagVerbose(opts.verbose);

  std::vector const flattener_args{
      opts.model_path, std::string{flags::instance_check_only},
      std::string{flags::include}, opts.ortools_include_dir};

  for (int i = 0; i < flattener_args.size(); i++) {
    if (auto const ok = flt.processOption(i, flattener_args); !ok) {
      return std::unexpected{
          InvalidFlag{.os = std::move(flattener_os),
                      .log = std::move(flattener_log)}
      };
    }
  }

  for (int i = 0; i < opts.infiles.size(); i++) {
    if (auto const ok = flt.processOption(i, opts.infiles); !ok) {
      return std::unexpected{
          InvalidFlag{.os = std::move(flattener_os),
                      .log = std::move(flattener_log)}
      };
    }
  }

  // TODO dump warnings

  try {
    flt.flatten("", "stdin");
  } catch (MiniZinc::Exception const& e) {
    return std::unexpected{
        MznParsingError{.os = std::move(flattener_os),
                        .log = std::move(flattener_log),
                        .msg = fmt::format("parsing failed:\n{}\n", e.msg())}
    };
  }

  // TODO dump warnings

  IR::Data data{};
  if (opts.print_ast) {
    auto& model = *flt.getEnv()->model();
    PrintModelVisitor vis{model, flt.getEnv()->envi(), data, opts.model_path};

    std::cout << "--- VAR DECLS ---" << std::endl;
    for (auto& var_decl : model.vardecls()) {
      vis.print_var_decl(var_decl.e(), 0);
    }

    // fmt::println("--- AST ---");
    //
    // MiniZinc::iter_items<PrintModelVisitor>(vis, &model);
    //
    // fmt::println("-----------");
  }

  return data;
}

} // namespace parser
