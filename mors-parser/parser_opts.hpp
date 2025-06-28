#pragma once

#include <clipp.h>

#include <expected>
#include <string>
#include <vector>

namespace parser {

struct ParserOpts {
  std::string model_path;
  std::vector<std::string> infiles;
  std::string stdlib_dir;
  std::string ortools_include_dir;
  bool verbose;
  bool help;

  static std::expected<ParserOpts, clipp::man_page> create(int, char **);

private:
  static ParserOpts init();
};

} // namespace parser
