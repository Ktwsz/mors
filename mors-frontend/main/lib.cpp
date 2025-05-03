#include "lib.hpp"

#include <iostream>

#include <minizinc/flattener.hh>

namespace frontend {

void say_hello() {
    auto const flt = new ::MiniZinc::Flattener(std::cout, std::cerr, std::string{""});

    std::cout << "aaa" << std::endl;
}

} // namespace frontend
