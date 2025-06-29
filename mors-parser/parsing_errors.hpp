#pragma once

#include <sstream>
#include <variant>

namespace parser {

struct InvalidFlag {
  std::stringstream os, log;
};

struct MznParsingError {
  std::stringstream os, log;
  std::string msg;
};

using ParsingError = std::variant<MznParsingError, InvalidFlag>;
// TODO: write visitor for error

} // namespace parser
