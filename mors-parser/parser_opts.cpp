#include "parser_opts.hpp"

#include <fmt/base.h>
#include <fmt/format.h>
#include <minizinc/file_utils.hh>
#include <minizinc/solver_config.hh>

#include <filesystem>

namespace parser {

auto defineCli(ParserOpts& opts) -> clipp::group {
  return (clipp::option("-h", "--help")
              .set(opts.help)
              .doc("Print this help message."),
          clipp::value("model.mzn", opts.model_path),
          clipp::opt_values("data.dzn", opts.infiles),
          (clipp::option("-o") & clipp::value("output file", opts.output_file)),
          (clipp::option("--stdlib-dir") & clipp::value("dir", opts.stdlib_dir))
              .doc(fmt::format("Default: {}", opts.stdlib_dir)),
          (clipp::option("-I", "--search-dir") &
           clipp::value("dir", opts.ortools_include_dir))
              .doc("Additionally search for included files in <dir>."),
          clipp::option("--verbose").set(opts.verbose),
          clipp::option("--print-ast").set(opts.print_ast));
}

auto ParserOpts::create(int argc, char** argv)
    -> std::expected<ParserOpts, clipp::man_page> {
  auto opts = ParserOpts::init();

  auto cli = defineCli(opts);
  if (!clipp::parse(argc, argv, cli) || opts.help) {
    return std::unexpected{clipp::make_man_page(cli, argv[0])};
  }

  return opts;
}

std::string ParserOpts::get_ortools_include_dir() const {
  return !ortools_include_dir.empty()
           ? ortools_include_dir
           : MiniZinc::FileUtils::file_path(stdlib_dir + "/solvers/cp-sat");
}

std::string ParserOpts::get_output_file() const {
    if (!output_file.empty())
        return output_file;

    std::filesystem::path output_path{model_path};
    output_path.replace_extension(".py");

    return output_path.string();
}

auto ParserOpts::init() -> ParserOpts {
  auto opts = ParserOpts{};

  auto solver_configs = MiniZinc::SolverConfigs(opts.logs);

  opts.stdlib_dir = solver_configs.mznlibDir();

  return opts;
}

void ParserOpts::dump_warnings() const {
  auto const logs_view = logs.view();
  if (!logs_view.empty())
    fmt::println("MiniZinc SolverConfigs returned warnings:\n{}", logs_view);
}
} // namespace parser
