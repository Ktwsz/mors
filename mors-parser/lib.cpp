#include "lib.hpp"
#include "ast_printer.hpp"

#include <iostream>

#include <minizinc/flattener.hh>
#include <minizinc/solver_config.hh>

namespace parser {

IR::Data main(std::vector<std::string> &args) {
  IR::Data data{};
  try {
    auto solver_configs = MiniZinc::SolverConfigs(std::cout);
    // TODO: log this for some debug flag
    // std::cout << "std path: " << solver_configs.mznlibDir() << std::endl;
    auto flt = MiniZinc::Flattener{std::cout, std::cerr, solver_configs.mznlibDir()};

    for (int i = 1; i < args.size(); i++) {
      flt.processOption(i, args);
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
