#pragma once

#include "ir.hpp"
#include "parser_opts.hpp"

namespace parser {

auto main(ParserOpts const&) -> IR::Data;

} // namespace parser
