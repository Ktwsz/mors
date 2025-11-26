#include "parsing_errors.hpp"

#include "utils.hpp"

#include <print>

namespace parser::err {

void print_message(Error const& e) {
  std::visit(
      utils::overloaded{[](MznParsingError const& err) {
                          std::println(
                              "Error encountered while parsing MiniZinc model");

                          std::println("Error message:");
                          if (!err.msg.empty())
                            std::println("{}", err.msg);

                          if (auto const view = err.os.view(); !view.empty()) {
                            std::println("Error logs:");
                            std::println("size: {}, {}", view.size(), view);
                          }

                          if (auto const view = err.log.view(); !view.empty()) {
                            std::println("Warning logs:");
                            std::println("size: {}, {}", view.size(), view);
                          }
                        },
                        [](InvalidFlag const& err) {
                          std::println("Error encountered while reading flags");

                          std::println("Error logs:");
                          if (auto const view = err.os.view(); !view.empty())
                            std::println("{}", view);

                          std::println("Warning logs:");
                          if (auto const view = err.log.view(); !view.empty())
                            std::println("{}", view);
                        },
                        [](Unsupported const& err) {
                          std::println("Unsupported feature encountered during "
                                       "translating MiniZinc AST:");

                          std::println("Message: {}", err.message);

                          if (err.location.empty())
                              return;

                          std::println("Stack trace:");
                          for (auto const& stack_frame : err.location)
                            std::println("{}", stack_frame);
                        }},
      e);
}
} // namespace parser::err
