#include "parser_opts.hpp"
#include "clipp.h"

#include <minizinc/file_utils.hh>
#include <minizinc/json_parser.hh>
#include <minizinc/solver_config.hh>

#include <filesystem>
#include <format>
#include <print>
#include <sstream>
#include <string>

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

auto help_command(ParserOpts& opts) -> clipp::parameter {
  return clipp::command("help")
      .set(opts.command, ParserOpts::Command::Help)
      .doc("print help message");
}

auto check_installation_command(ParserOpts& opts) -> clipp::group {
  return (clipp::command("check-installation")
              .set(opts.command, ParserOpts::Command::CheckInstallation) |
          clipp::command("ci").set(opts.command,
                                   ParserOpts::Command::CheckInstallation))
      .doc("logs paths where MiniZinc looks for stdlib and solver "
           "configuration");
}

auto build_command(ParserOpts& opts) -> clipp::parameter {
  return clipp::command("build")
      .set(opts.command, ParserOpts::Command::Build)
      .doc("build the given model");
}

auto full_build_command(ParserOpts& opts) -> clipp::group {
  using namespace clipp;
  return (
      build_command(opts),
      opt_value("model.mzn", opts.model_path) &
          opt_values("data.dzn", opts.infiles),
      (option("-o") & value("output file", opts.output_file)),
      (option("--runtime-parameters").set(opts.runtime_parameters)),
      (option("--stdlib-dir") & value("dir", opts.stdlib_dir))
          .doc("Path to MiniZinc standard library directory"),
      repeatable(option("-I", "--search-dir") & value("dir", opts.include_dirs))
          .doc("Additionally search for included files in <dir>. Use this flag "
               "to manually specify the location of OR-Tools redefinitions."),
      option("--print-ast").set(opts.print_ast),
      option("-h", "--help").set(opts.help).doc("Print this help message."));
}

auto define_help_cli(ParserOpts& opts) -> clipp::group {
  return build_command(opts) | help_command(opts) |
         check_installation_command(opts);
}

auto define_cli(ParserOpts& opts) -> clipp::group {
  return full_build_command(opts) | help_command(opts) |
         check_installation_command(opts);
}

auto make_build_help(ParserOpts& opts) -> std::string {
  std::stringstream s;
  s << clipp::make_man_page(full_build_command(opts));
  return s.str();
}

auto make_command_help(ParserOpts& opts) -> std::string {
  auto fmt = clipp::doc_formatting{}
                 .first_column(2)
                 .doc_column(16)
                 .alternatives_min_split_size(2);

  auto help_cli = define_help_cli(opts);

  return std::format("mors\n\nUsage:\n{}\n\nCommands:\n{}\n",
                     clipp::usage_lines(help_cli, "mors", fmt).str(),
                     clipp::documentation(help_cli, fmt).str());
}

auto ParserOpts::create(int argc, char** argv)
    -> std::expected<ParserOpts, std::string> {
  auto opts = ParserOpts{};

  auto cli = define_cli(opts);
  if (!clipp::parse(argc, argv, cli) || opts.command == Command::Help) {
    if (opts.command != Command::Help)
      std::println("Error parsing the command.");

    if (opts.command == Command::Build)
      return std::unexpected{make_build_help(opts)};

    return std::unexpected{make_command_help(opts)};
  }

  if (opts.command == ParserOpts::Command::Build) {
    if (opts.help)
      return std::unexpected{make_build_help(opts)};

    if (opts.model_path.empty()) {
      return std::unexpected{std::format("Model path cannot be empty.\n{}",
                                         make_build_help(opts))};
    }

    opts.finalize();

    if (auto errors = opts.validate_model_path(); errors)
      return std::unexpected{*errors};

    if (auto errors = opts.validate_data_file_path(); errors)
      return std::unexpected{*errors};
  }

  return opts;
}

std::string ParserOpts::get_output_file() const {
  if (!output_file.empty())
    return output_file;

  std::filesystem::path output_path{model_path};
  output_path.replace_extension(".py");

  return output_path.string();
}

void ParserOpts::finalize() {
  model_file = std::filesystem::path{model_path}.filename();

  try {
    auto solver_configs = MiniZinc::SolverConfigs(logs);

    solver_configs.populate(std::cout);
    auto ortools_conf = solver_configs.config("cp-sat");

    if (stdlib_dir.empty())
      stdlib_dir = solver_configs.mznlibDir();

    include_dirs.push_back(ortools_conf.mznlibResolved());
  } catch (...) {
  }
}

void ParserOpts::dump_warnings() const {
  auto const logs_view = logs.view();
  if (!logs_view.empty())
    std::println("MiniZinc SolverConfigs returned warnings:\n{}", logs_view);
}

auto ParserOpts::validate_model_path() const -> std::optional<std::string> {
  std::filesystem::path f{model_file};
  if (!f.has_extension() || f.extension() != ".mzn")
    return "Model file must have .mzn extension";
  return std::nullopt;
}

auto ParserOpts::validate_data_file_path() const -> std::optional<std::string> {
  for (auto const& infile : infiles) {
    std::filesystem::path f{infile};

    if (!f.has_extension() || f.extension() != ".dzn")
      return "Data files must have .dzn extension";
  }
  return std::nullopt;
}

void ParserOpts::run_installation_check() {
  std::println("==== Installation check ====");
  std::vector<std::string> _solverPath;
  std::string _mznlibDir;
  auto paths = MiniZinc::FileUtils::get_env_list("MZN_SOLVER_PATH");

  if (!paths.empty())
    std::println("Solver paths in environment variable MZN_SOLVER_PATH:");
  else
    std::println("Environment variable MZN_SOLVER_PATH empty");

  for (std::string const& s : paths)
    std::println("- {}", s);

  _solverPath.insert(_solverPath.end(), paths.begin(), paths.end());
  std::string userConfigDir = MiniZinc::FileUtils::user_config_dir();
  if (MiniZinc::FileUtils::directory_exists(userConfigDir + "/solvers")) {
    _solverPath.push_back(userConfigDir + "/solvers");
  }

  std::vector<std::string> configFiles(
      {MiniZinc::FileUtils::global_config_file(),
       MiniZinc::FileUtils::user_config_file()});

  for (auto& cf : configFiles) {
    std::println("Looking for config file: {}", cf);
    if (cf.empty()) {
      std::println("Skipping, reason: empty path");
      continue;
    }
    if (!MiniZinc::FileUtils::file_exists(cf)) {
      std::println("Skipping, reason: file doesnt exist");
      continue;
    }
    if (!MiniZinc::JSONParser::fileIsJSON(cf)) {
      std::println("Skipping, reason: not a JSON file");
      continue;
    }

    std::println("File ok, proceeding");

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
              std::println("- solver path: {}", s);
              _solverPath.push_back(s);
            }
          } else if (ai->id() == "mzn_lib_dir") {
            _mznlibDir = get_string(ai);
            std::println("- stdlib path: {}", _mznlibDir);
          }
        }
      }
    } catch (MiniZinc::ConfigException& e) {
      std::println("Parsing JSON file failed with message: {}", e.msg());
    } catch (MiniZinc::Exception& e) {
    }
  }

  std::println("Parsing config files finished");

  if (_mznlibDir.empty()) {
    _mznlibDir =
        MiniZinc::FileUtils::file_path(MiniZinc::FileUtils::share_directory());
    std::println("stdlib path not found, trying path {}", _mznlibDir);
  }
  if (!_mznlibDir.empty()) {
    _solverPath.push_back(_mznlibDir + "/solvers");
    std::println("- solver path: {}", _solverPath.back());
  }
#ifndef _MSC_VER
  if (_mznlibDir != "/usr/local/share/minizinc" &&
      MiniZinc::FileUtils::directory_exists("/usr/local/share")) {
    _solverPath.emplace_back("/usr/local/share/minizinc/solvers");
    std::println("- solver path: {}", _solverPath.back());
  }
  if (_mznlibDir != "/usr/share/minizinc" &&
      MiniZinc::FileUtils::directory_exists("/usr/share")) {
    _solverPath.emplace_back("/usr/share/minizinc/solvers");
    std::println("- solver path: {}", _solverPath.back());
  }
  if (_mznlibDir != "/home/linuxbrew/.linuxbrew/share/minizinc" &&
      MiniZinc::FileUtils::directory_exists(
          "/home/linuxbrew/.linuxbrew/share")) {
    _solverPath.emplace_back(
        "/home/linuxbrew/.linuxbrew/share/minizinc/solvers");
    std::println("- solver path: {}", _solverPath.back());
  }
  if (_mznlibDir != "/opt/homebrew/share/minizinc" &&
      MiniZinc::FileUtils::directory_exists("/opt/homebrew/share")) {
    _solverPath.emplace_back("/opt/homebrew/share/minizinc/solvers");
    std::println("- solver path: {}", _solverPath.back());
  }
#endif

  auto progpath_share = MiniZinc::FileUtils::file_path(
      MiniZinc::FileUtils::progpath() + "/share/minizinc");
  if (progpath_share != _mznlibDir &&
      MiniZinc::FileUtils::directory_exists(progpath_share)) {
    _solverPath.emplace_back(progpath_share + "/solvers");
    std::println("- solver path: {}", _solverPath.back());
  }

  std::println("==== Installation check finished ====");
  std::println("Final stdlib path: {}", _mznlibDir);
  std::println("Final list of solver paths:");
  for (std::string const& s : _solverPath)
    std::println("- {}", s);
}
} // namespace parser
