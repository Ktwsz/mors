#include "lib.hpp"
#include "ast_printer.hpp"

#include <iostream>

#include <minizinc/file_utils.hh>
#include <minizinc/flattener.hh>
#include <minizinc/solver_config.hh>
#include <string_view>

namespace parser {
namespace flags {
using namespace std::string_view_literals;

constexpr std::string_view instance_check_only = "--instance-check-only"sv;
constexpr std::string_view include = "-I"sv;
} // namespace flags

IR::Data main(ParserOpts &opts) {
  IR::Data data{};
  try {
    if (opts.stdlib_dir.empty()) {
      auto solver_configs = MiniZinc::SolverConfigs(std::cout);
      opts.stdlib_dir = solver_configs.mznlibDir();
    }
    // TODO: log this for some debug flag
    std::cout << "std path: " << opts.stdlib_dir << std::endl;

    if (opts.ortools_include_dir.empty()) {
      opts.ortools_include_dir =
          MiniZinc::FileUtils::file_path(opts.stdlib_dir + "/solvers/cp-sat");
    }
    // TODO: log this for some debug flag
    std::cout << "OR-Tools path: " << opts.ortools_include_dir << std::endl;

    auto flt = MiniZinc::Flattener{std::cout, std::cerr, opts.stdlib_dir};

    std::vector<std::string> flattener_args{
        opts.model_path, std::string{flags::instance_check_only},
        std::string{flags::include}, opts.ortools_include_dir};

    for (int i = 0; i < flattener_args.size(); i++) {
      flt.processOption(i, flattener_args);
    }

    for (int i = 0; i < opts.infiles.size(); i++) {
      flt.processOption(i, opts.infiles);
    }

    flt.flatten("", "stdin");

    auto &model = *flt.getEnv()->model();

    std::cout << "--- AST ---" << std::endl;

    PrintModelVisitor vis{model, flt.getEnv()->envi(), data};
    MiniZinc::iter_items<PrintModelVisitor>(vis, &model);

    std::cout << "-----------" << std::endl;
  } catch (MiniZinc::Exception const &e) {
    std::cout << "parsing failed: " << std::endl;
    std::cout << e.msg() << std::endl;
  }
  return data;
}

} // namespace parser
