#pragma once

#include <cctype>
#include <fstream>
#include <functional>
#include <ranges>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

#include <fmt/format.h>

namespace parser::err {

struct InvalidFlag {
  std::ostringstream os, log;
};

struct MznParsingError {
  std::ostringstream os, log;
  std::string msg;
};

struct Unsupported {
  struct Location {
    std::string filename;
    size_t first_line;
    size_t first_column;
    size_t last_line;
    size_t last_column;

    auto is_index_within_highlight(size_t line, size_t ix) const -> bool {
      if (line < first_line || line > last_line)
        return false;

      if (first_line == last_line)
        return first_column <= ix && ix <= last_column;

      if (line == first_line)
        return first_column <= ix;
      if (line == last_line)
        return ix <= last_column;

      return true;
    }
  };

  std::vector<Location> location{};
  std::string message;

  void add_location(Location loc) { location.push_back(loc); }
};

using Error = std::variant<MznParsingError, InvalidFlag, Unsupported>;

void print_message(Error const& e);

} // namespace parser::err

template <> class fmt::v11::formatter<parser::err::Unsupported::Location> {
public:
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
  template <typename Context>
  constexpr auto format(parser::err::Unsupported::Location const& location,
                        Context& ctx) const {
    format_to(ctx.out(), "{}:{}.{}", location.filename, location.first_line,
              location.first_column);
    if (location.first_line != location.last_line)
      format_to(ctx.out(), "-{}.{}\n", location.last_line,
                location.last_column);
    else if (location.first_column != location.last_column)
      format_to(ctx.out(), "-{}\n", location.last_column);

    std::ifstream file{location.filename, std::ios::in | std::ios::binary};
    if (!file.is_open())
      return ctx.out();

    std::string line;
    size_t line_ctr = 1;
    size_t max_line_width = floor(log10f(static_cast<float>(location.last_line)) + 1);

    while (line_ctr <= location.last_line && std::getline(file, line)) {
      if (line_ctr >= location.first_line) {
        format_to(ctx.out(), " {:>{}} |", line_ctr, max_line_width);
        size_t highlight_begin =
            std::find_if(line.begin(), line.end(), std::not_fn(isspace)) -
            line.begin();

        size_t highlight_end =
            std::find_if(line.rbegin(), line.rend(), std::not_fn(isspace)) -
            line.rend();

        format_to(ctx.out(), "{}\n", line);
        format_to(ctx.out(), " {: >{}} |", "", max_line_width);

        for (auto i : std::views::iota(0u, line.size()))
          format_to(ctx.out(), "{}",
                    highlight_begin <= i && i <= highlight_end &&
                            location.is_index_within_highlight(line_ctr, i+1)
                        ? '~'
                        : ' ');

        format_to(ctx.out(), "\n");
      }
      line_ctr++;
    }

    return ctx.out();
  }
};
