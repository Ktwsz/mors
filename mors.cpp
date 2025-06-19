#include "lib.hpp"

#include <iostream>

#include <clipp.h>
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

namespace py = pybind11;

int main(int argc, char **argv) {

  std::string model_path;
  std::vector<std::string> infiles;
  std::string stdlib_dir;
  bool verbose;
  bool help;

  auto cli =
      (clipp::value("model.mzn", model_path),
       clipp::opt_values("data.dzn", infiles),
       clipp::option("--stdlib-dir") & clipp::value("dir", stdlib_dir),
       clipp::option("--verbose").set(verbose),
       clipp::option("-h", "--help").set(help).doc("Print this help message."));

  if (!clipp::parse(argc, argv, cli))
    std::cout << clipp::make_man_page(cli, argv[0]);

  std::cout << help << std::endl;

  if (help)
    std::cout << clipp::make_man_page(cli, argv[0]);

  return 0;

  std::vector<std::string> args{"aaa", "bbb"};
  auto const result = parser::main(args);

  py::scoped_interpreter _;

  using namespace py::literals;

  try {
    py::function hello_world =
        py::module_::import("emitter").attr("hello_world");
    auto parser_python = py::module::import("ir_python");

    hello_world(result);

  } catch (py::error_already_set e) {
    std::cout << "python binding err:" << std::endl;
    std::cout << e.what() << std::endl;
  }
}
