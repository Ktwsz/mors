#pragma once

#include "ir.hpp"
#include "parser_opts.hpp"
#include "parsing_errors.hpp"
#include <expected>

namespace parser {

auto main(ParserOpts const&) -> std::expected<IR::Data, err::Error>;

} // namespace parser
