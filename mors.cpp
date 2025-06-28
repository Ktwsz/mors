#include "lib.hpp"
#include "parser_opts.hpp"

#include <iostream>

#include <fmt/base.h>
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

namespace py = pybind11;

int main(int argc, char** argv) {
  auto const opts = parser::ParserOpts::create(argc, argv);

  if (!opts) {
    std::cout << opts.error() << std::endl;
    return 0;
  }

  auto const result = parser::main(opts.value());

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
