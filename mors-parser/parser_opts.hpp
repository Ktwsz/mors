#pragma once

#include <expected>
#include <sstream>
#include <string>
#include <vector>

#include <clipp.h>

namespace parser {

struct ParserOpts {
  enum class Command {
    Help,
    CheckInstallation,
    Build,
  };
  Command command;
  std::string model_path;
  std::string model_file;
  std::vector<std::string> infiles;
  std::string stdlib_dir;
  std::vector<std::string> include_dirs;
  bool print_ast;
  bool runtime_parameters;
  bool help;

  std::ostringstream logs;

  static auto create(int, char**) -> std::expected<ParserOpts, std::string>;
  static void run_installation_check();

  void dump_warnings() const;

  auto validate_model_path() const -> std::optional<std::string>;
  auto validate_data_file_path() const -> std::optional<std::string>;

  std::string get_output_file() const;

  friend auto full_build_command(ParserOpts& opts) -> clipp::group;

private:
  void finalize();

  std::string output_file;
};

} // namespace parser
