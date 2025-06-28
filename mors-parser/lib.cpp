#include "lib.hpp"
#include "ast_printer.hpp"

#include <iostream>
#include <string_view>

#include <fmt/base.h>
#include <minizinc/flattener.hh>

namespace parser {
namespace flags {
using namespace std::string_view_literals;

constexpr std::string_view instance_check_only = "--instance-check-only"sv;
constexpr std::string_view include = "-I"sv;
} // namespace flags

auto main(ParserOpts const& opts) -> IR::Data {
  IR::Data data{};
  try {
    // TODO: log this for some debug flag
    fmt::println("std path: {}", opts.stdlib_dir);

    // TODO: log this for some debug flag
    fmt::println("OR-Tools path: {}", opts.ortools_include_dir);

    auto flt = MiniZinc::Flattener{std::cout, std::cerr, opts.stdlib_dir};

    std::vector<std::string> const flattener_args{
        opts.model_path, std::string{flags::instance_check_only},
        std::string{flags::include}, opts.ortools_include_dir};

    for (int i = 0; i < flattener_args.size(); i++) {
      flt.processOption(i, flattener_args);
    }

    for (int i = 0; i < opts.infiles.size(); i++) {
      flt.processOption(i, opts.infiles);
    }

    flt.flatten("", "stdin");

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
  } catch (MiniZinc::Exception const& e) {
    fmt::println("parsing failed: ");
    fmt::println("{}", e.msg());
  }
  return data;
}

} // namespace parser
