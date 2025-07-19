#pragma once

#include "ast.hpp"
#include "parser_opts.hpp"
#include "parsing_errors.hpp"

#include <expected>

namespace parser {

auto main(ParserOpts const&) -> std::expected<ast::Tree, err::Error>;

} // namespace parser
