#pragma once

#include <string>
#include <vector>

namespace parser {

struct ParserOpts {
  std::string model_path;
  std::vector<std::string> infiles;
  std::string stdlib_dir;
  std::string ortools_include_dir;
  bool verbose;
};

} // namespace parser
