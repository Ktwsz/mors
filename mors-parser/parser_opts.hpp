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
  std::string ortools_include_dir;
  bool verbose;
  bool print_ast;
  bool help;

  std::ostringstream logs;

  static auto create(int, char**) -> std::expected<ParserOpts, clipp::man_page>;

  void dump_warnings() const;

private:
  static auto init() -> ParserOpts;
};

} // namespace parser
