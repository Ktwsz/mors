#include "parser_opts.hpp"

#include <fmt/base.h>
#include <fmt/format.h>

#include <minizinc/file_utils.hh>
#include <minizinc/json_parser.hh>
#include <minizinc/solver_config.hh>

#include <filesystem>

namespace parser {

namespace {
std::string get_string(MiniZinc::AssignI* ai) {
  if (auto* sl =
          MiniZinc::Expression::dynamicCast<MiniZinc::StringLit>(ai->e())) {
    return std::string(sl->v().c_str(), sl->v().size());
  }
  throw MiniZinc::ConfigException(
      "invalid configuration item (right hand side must be string)");
}
std::vector<std::string> get_string_list(MiniZinc::AssignI* ai) {
  if (auto* al =
          MiniZinc::Expression::dynamicCast<MiniZinc::ArrayLit>(ai->e())) {
    std::vector<std::string> ret;
    for (unsigned int i = 0; i < al->size(); i++) {
      if (auto* sl = MiniZinc::Expression::dynamicCast<MiniZinc::StringLit>(
              (*al)[i])) {
        ret.emplace_back(sl->v().c_str(), sl->v().size());
      } else {
        throw MiniZinc::ConfigException(
            "invalid configuration item (right hand side "
            "must be a list of strings)");
      }
    }
    return ret;
  }
  throw MiniZinc::ConfigException(
      "invalid configuration item (right hand side must be a list of strings)");
}
} // namespace

auto defineCli(ParserOpts& opts) -> clipp::group {
  return (clipp::opt_value("model.mzn", opts.model_path),
          clipp::opt_values("data.dzn", opts.infiles),
          (clipp::option("-o") & clipp::value("output file", opts.output_file)),
          (clipp::option("--runtime-parameters").set(opts.runtime_parameters)),
          (clipp::option("--stdlib-dir") & clipp::value("dir", opts.stdlib_dir))
              .doc(fmt::format("Default: {}", opts.stdlib_dir)),
          (clipp::option("-I", "--search-dir") &
           clipp::value("dir", opts.ortools_include_dir))
              .doc("Additionally search for included files in <dir>."),
          clipp::option("--verbose").set(opts.verbose),
          clipp::option("--print-ast").set(opts.print_ast),
          clipp::option("-h", "--help")
              .set(opts.help)
              .doc("Print this help message."),
          clipp::option("--installation-check")
              .set(opts.installation_check)
              .doc("Print this help message."));
}

auto ParserOpts::create(int argc, char** argv)
    -> std::expected<ParserOpts, clipp::man_page> {
  auto opts = ParserOpts{};

  auto cli = defineCli(opts);
  if (!clipp::parse(argc, argv, cli) || opts.help) {
    return std::unexpected{clipp::make_man_page(cli, argv[0])};
  }

  if (!opts.finalize())
    return std::unexpected{clipp::make_man_page(cli, argv[0])};

  if (opts.model_path.empty() && !opts.installation_check)
    return std::unexpected{clipp::make_man_page(cli, argv[0])};

  return opts;
}

std::string ParserOpts::get_output_file() const {
  if (!output_file.empty())
    return output_file;

  std::filesystem::path output_path{model_path};
  output_path.replace_extension(".py");

  return output_path.string();
}

auto ParserOpts::finalize() -> bool {
  // TODO - try catch ...

  auto solver_configs = MiniZinc::SolverConfigs(logs);

  solver_configs.populate(std::cout);
  auto ortools_conf = solver_configs.config("cp-sat");

  stdlib_dir = solver_configs.mznlibDir();
  ortools_include_dir = ortools_conf.mznlibResolved();

  return true;
}

void ParserOpts::dump_warnings() const {
  auto const logs_view = logs.view();
  if (!logs_view.empty())
    fmt::println("MiniZinc SolverConfigs returned warnings:\n{}", logs_view);
}

void ParserOpts::run_installation_check() {
  fmt::println("==== Installation check ====");
  std::vector<std::string> _solverPath;
  std::string _mznlibDir;
  auto paths = MiniZinc::FileUtils::get_env_list("MZN_SOLVER_PATH");

  if (!paths.empty())
      fmt::println("Solver paths in environment variable MZN_SOLVER_PATH:");
  else
      fmt::println("Environment variable MZN_SOLVER_PATH empty");

  for (std::string const& s : paths)
    fmt::println("- {}", s);

  _solverPath.insert(_solverPath.end(), paths.begin(), paths.end());
  std::string userConfigDir = MiniZinc::FileUtils::user_config_dir();
  if (MiniZinc::FileUtils::directory_exists(userConfigDir + "/solvers")) {
    _solverPath.push_back(userConfigDir + "/solvers");
  }

  std::vector<std::string> configFiles(
      {MiniZinc::FileUtils::global_config_file(),
       MiniZinc::FileUtils::user_config_file()});

  for (auto& cf : configFiles) {
    fmt::println("Looking for config file: {}", cf);
    if (cf.empty()) {
      fmt::println("Skipping, reason: empty path");
      continue;
    }
    if (!MiniZinc::FileUtils::file_exists(cf)) {
      fmt::println("Skipping, reason: file doesnt exist");
      continue;
    }
    if (!MiniZinc::JSONParser::fileIsJSON(cf)) {
      fmt::println("Skipping, reason: not a JSON file");
      continue;
    }

    fmt::println("File ok, proceeding");

    try {
      MiniZinc::Env userconfenv;
      MiniZinc::GCLock lock;
      MiniZinc::JSONParser jp(userconfenv.envi());
      MiniZinc::Model m;
      jp.parse(&m, cf, false);
      for (auto& i : m) {
        if (auto* ai = i->dynamicCast<MiniZinc::AssignI>()) {
          if (ai->id() == "mzn_solver_path") {
            std::vector<std::string> sp = get_string_list(ai);
            for (const auto& s : sp) {
              fmt::println("- solver path: {}", s);
              _solverPath.push_back(s);
            }
          } else if (ai->id() == "mzn_lib_dir") {
            _mznlibDir = get_string(ai);
            fmt::println("- stdlib path: {}", _mznlibDir);
          }
        }
      }
    } catch (MiniZinc::ConfigException& e) {
      fmt::println("Parsing JSON file failed with message: {}", e.msg());
    } catch (MiniZinc::Exception& e) {
    }
  }

  fmt::println("Parsing config files finished");

  if (_mznlibDir.empty()) {
    _mznlibDir =
        MiniZinc::FileUtils::file_path(MiniZinc::FileUtils::share_directory());
    fmt::println("stdlib path not found, trying path {}", _mznlibDir);
  }
  if (!_mznlibDir.empty()) {
    _solverPath.push_back(_mznlibDir + "/solvers");
    fmt::println("- solver path: {}", _solverPath.back());
  }
#ifndef _MSC_VER
  if (_mznlibDir != "/usr/local/share/minizinc" &&
      MiniZinc::FileUtils::directory_exists("/usr/local/share")) {
    _solverPath.emplace_back("/usr/local/share/minizinc/solvers");
    fmt::println("- solver path: {}", _solverPath.back());
  }
  if (_mznlibDir != "/usr/share/minizinc" &&
      MiniZinc::FileUtils::directory_exists("/usr/share")) {
    _solverPath.emplace_back("/usr/share/minizinc/solvers");
    fmt::println("- solver path: {}", _solverPath.back());
  }
  if (_mznlibDir != "/home/linuxbrew/.linuxbrew/share/minizinc" &&
      MiniZinc::FileUtils::directory_exists(
          "/home/linuxbrew/.linuxbrew/share")) {
    _solverPath.emplace_back(
        "/home/linuxbrew/.linuxbrew/share/minizinc/solvers");
    fmt::println("- solver path: {}", _solverPath.back());
  }
  if (_mznlibDir != "/opt/homebrew/share/minizinc" &&
      MiniZinc::FileUtils::directory_exists("/opt/homebrew/share")) {
    _solverPath.emplace_back("/opt/homebrew/share/minizinc/solvers");
    fmt::println("- solver path: {}", _solverPath.back());
  }
#endif

  auto progpath_share = MiniZinc::FileUtils::file_path(
      MiniZinc::FileUtils::progpath() + "/share/minizinc");
  if (progpath_share != _mznlibDir &&
      MiniZinc::FileUtils::directory_exists(progpath_share)) {
    _solverPath.emplace_back(progpath_share + "/solvers");
    fmt::println("- solver path: {}", _solverPath.back());
  }

  fmt::println("==== Installation check finished ====");
  fmt::println("Final stdlib path: {}", _mznlibDir);
  fmt::println("Final list of solver paths:");
  for (std::string const& s : _solverPath)
    fmt::println("- {}", s);
}
} // namespace parser
