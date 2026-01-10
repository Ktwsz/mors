#include "lib.hpp"
#include "parser_opts.hpp"

#include <print>

#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

namespace py = pybind11;

int main(int argc, char** argv) {
  auto const opts = parser::ParserOpts::create(argc, argv);

  if (!opts) {
    std::println("{}", opts.error());
    return 1;
  }

  if (opts->command == parser::ParserOpts::Command::CheckInstallation) {
    parser::ParserOpts::run_installation_check();
    return 0;
  }

  opts->dump_warnings();
  auto const result = parser::main(*opts);

  if (!result) {
    parser::err::print_message(result.error());
    return 1;
  }

  if (opts->print_ast)
    return 0;

  py::scoped_interpreter _;

  using namespace py::literals;

  try {
    py::function emitter_main = py::module_::import("mors_emitter").attr("main");
    auto parser_python = py::module::import("mors_ir");

    emitter_main(*result, opts->get_output_file());

  } catch (py::error_already_set const& e) {
    std::println("python binding err:");
    std::println("{}", e.what());
  }
}
