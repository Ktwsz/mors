#pragma once

#include <expected>
#include <sstream>
#include <string>
#include <vector>

#include <clipp.h>

namespace parser {

struct ParserOpts {
  std::string model_path;
  std::vector<std::string> infiles;
  std::string stdlib_dir;
  bool verbose;
  bool print_ast;
  bool help;

  std::ostringstream logs;

  static auto create(int, char**) -> std::expected<ParserOpts, clipp::man_page>;

  void dump_warnings() const;

  std::string get_ortools_include_dir() const;
  std::string get_output_file() const;

  friend auto defineCli(ParserOpts& opts) -> clipp::group;

private:
  static auto init() -> ParserOpts;
  std::string ortools_include_dir;
  std::string output_file;
};

} // namespace parser
