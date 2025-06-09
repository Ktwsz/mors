#include "lib.hpp"

#include <iostream>
#include <ranges>

#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

namespace py = pybind11;

int main(int argc, char **argv) {
  auto args =
      std::ranges::iota_view(1, argc) |
      std::views::transform([&argv](int i) { return std::string{argv[i]}; }) |
      std::ranges::to<std::vector<std::string>>();

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
