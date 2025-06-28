#include "parser_opts.hpp"

#include <fmt/format.h>
#include <minizinc/file_utils.hh>
#include <minizinc/solver_config.hh>

namespace parser {

namespace {
clipp::group defineCli(ParserOpts &opts) {
  return (
      clipp::value("model.mzn", opts.model_path),
      clipp::opt_values("data.dzn", opts.infiles),
      (clipp::option("--stdlib-dir") & clipp::value("dir", opts.stdlib_dir))
          .doc(fmt::format("Default: {}", opts.stdlib_dir)),
      (clipp::option("-I", "--search-dir") &
       clipp::value("dir", opts.ortools_include_dir))
          .doc(fmt::format("Additionally search for included files in <dir>. Default: {}",
               opts.ortools_include_dir)),
      clipp::option("--verbose").set(opts.verbose),
      clipp::option("-h", "--help")
          .set(opts.help)
          .doc("Print this help message."));
}
} // namespace

std::expected<ParserOpts, clipp::man_page> ParserOpts::create(int argc,
                                                              char **argv) {
  auto opts = ParserOpts::init();

  auto cli = defineCli(opts);
  if (!clipp::parse(argc, argv, cli) || opts.help) {
    return std::unexpected{clipp::make_man_page(cli, argv[0])};
  }

  return opts;
}

ParserOpts ParserOpts::init() {
  auto opts = ParserOpts{};

  auto solver_configs = MiniZinc::SolverConfigs(std::cout);

  opts.stdlib_dir = solver_configs.mznlibDir();

  opts.ortools_include_dir =
      MiniZinc::FileUtils::file_path(opts.stdlib_dir + "/solvers/cp-sat");

  return opts;
}
} // namespace parser
