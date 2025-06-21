#include "lib.hpp"
#include "parser_opts.hpp"

#include <iostream>

#include <clipp.h>
#include <fmt/base.h>
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

namespace py = pybind11;

int main(int argc, char **argv) {

  parser::ParserOpts opts{};
  bool help;

  auto cli =
      (clipp::value("model.mzn", opts.model_path),
       clipp::opt_values("data.dzn", opts.infiles),
       clipp::option("--stdlib-dir") & clipp::value("dir", opts.stdlib_dir),
       (clipp::option("-I", "--search-dir") &
        clipp::value("dir", opts.ortools_include_dir))
           .doc("Additionally search for included files in <dir>."),
       clipp::option("--verbose").set(opts.verbose),
       clipp::option("-h", "--help").set(help).doc("Print this help message."));

  if (!clipp::parse(argc, argv, cli) || help) {
    std::cout << clipp::make_man_page(cli, argv[0]);
    return 0;
  }

  auto const result = parser::main(opts);

  py::scoped_interpreter _;

  using namespace py::literals;

  try {
    py::function hello_world =
        py::module_::import("emitter").attr("hello_world");
    auto parser_python = py::module::import("ir_python");

    hello_world(result);

  } catch (py::error_already_set e) {
    fmt::println("python binding err:");
    fmt::println("{}", e.what());
  }
}
