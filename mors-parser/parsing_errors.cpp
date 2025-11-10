#include "parsing_errors.hpp"

#include "utils.hpp"

#include <fmt/base.h>

namespace parser::err {

void print_message(Error const& e) {
  std::visit(
      utils::overloaded{[](MznParsingError const& err) {
                          fmt::println(
                              "Error encountered while parsing MiniZinc model");

                          fmt::println("Error message:");
                          if (!err.msg.empty())
                            fmt::println("{}", err.msg);

                          if (auto const view = err.os.view(); !view.empty()) {
                            fmt::println("Error logs:");
                            fmt::println("size: {}, {}", view.size(), view);
                          }

                          if (auto const view = err.log.view(); !view.empty()) {
                            fmt::println("Warning logs:");
                            fmt::println("size: {}, {}", view.size(), view);
                          }
                        },
                        [](InvalidFlag const& err) {
                          fmt::println("Error encountered while reading flags");

                          fmt::println("Error logs:");
                          if (auto const view = err.os.view(); !view.empty())
                            fmt::println("{}", view);

                          fmt::println("Warning logs:");
                          if (auto const view = err.log.view(); !view.empty())
                            fmt::println("{}", view);
                        },
                        [](Unsupported const& err) {
                          fmt::println("Unsupported feature encountered during "
                                       "translating MiniZinc AST:");

                          fmt::println("Message: {}", err.message);

                          if (err.location.empty())
                              return;

                          fmt::println("Stack trace:");
                          for (auto const& stack_frame : err.location)
                            fmt::println("{}", stack_frame);
                        }},
      e);
}
} // namespace parser::err
