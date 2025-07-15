#pragma once

#include <sstream>
#include <variant>

namespace parser::err {

struct InvalidFlag {
  std::ostringstream os, log;
};

struct MznParsingError {
  std::ostringstream os, log;
  std::string msg;
};

using Error = std::variant<MznParsingError, InvalidFlag>;

void print_message(Error const& e);

} // namespace parser::err
