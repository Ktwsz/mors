#include "lib.hpp"
#include "parser_opts.hpp"

#include <sstream>

#include <fmt/base.h>
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

namespace py = pybind11;

int main(int argc, char** argv) {
  auto const opts = parser::ParserOpts::create(argc, argv);

  if (!opts) {
    std::stringstream man_page;
    man_page << opts.error();
    fmt::println("{}", man_page.view());
    return 0;
  }

  opts->dump_warnings();
  auto const result = parser::main(*opts);

  if (!result) {
      // TODO: dump error
      return 0;
  }

  if (opts->print_ast)
      return 0;

  py::scoped_interpreter _;

  using namespace py::literals;

  try {
    py::function hello_world =
        py::module_::import("emitter").attr("hello_world");
    auto parser_python = py::module::import("ir_python");

    hello_world(*result);

  } catch (py::error_already_set const& e) {
    fmt::println("python binding err:");
    fmt::println("{}", e.what());
  }
}
