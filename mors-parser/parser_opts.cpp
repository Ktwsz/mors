#include "parser_opts.hpp"

#include <fmt/base.h>
#include <fmt/format.h>
#include <minizinc/file_utils.hh>
#include <minizinc/solver_config.hh>

namespace parser {

namespace {
auto defineCli(ParserOpts& opts) -> clipp::group {
  return (
      clipp::option("-h", "--help")
          .set(opts.help)
          .doc("Print this help message."),
      clipp::value("model.mzn", opts.model_path),
      clipp::opt_values("data.dzn", opts.infiles),
      (clipp::option("--stdlib-dir") & clipp::value("dir", opts.stdlib_dir))
          .doc(fmt::format("Default: {}", opts.stdlib_dir)),
      (clipp::option("-I", "--search-dir") &
       clipp::value("dir", opts.ortools_include_dir))
          .doc(fmt::format(
              "Additionally search for included files in <dir>. Default: {}",
              opts.ortools_include_dir)),
      clipp::option("--verbose").set(opts.verbose),
      clipp::option("--print-ast").set(opts.print_ast));
}
} // namespace

auto ParserOpts::create(int argc, char** argv)
    -> std::expected<ParserOpts, clipp::man_page> {
  auto opts = ParserOpts::init();

  auto cli = defineCli(opts);
  if (!clipp::parse(argc, argv, cli) || opts.help) {
    return std::unexpected{clipp::make_man_page(cli, argv[0])};
  }

  return opts;
}

auto ParserOpts::init() -> ParserOpts {
  auto opts = ParserOpts{};

  auto solver_configs = MiniZinc::SolverConfigs(opts.logs);

  opts.stdlib_dir = solver_configs.mznlibDir();

  opts.ortools_include_dir =
      MiniZinc::FileUtils::file_path(opts.stdlib_dir + "/solvers/cp-sat");

  return opts;
}

void ParserOpts::dump_warnings() const {
  auto const logs_view = logs.view();
  if (!logs_view.empty())
    fmt::println("MiniZinc SolverConfigs returned warnings:\n{}", logs_view);
}
} // namespace parser
