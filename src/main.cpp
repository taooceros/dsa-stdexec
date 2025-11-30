#include <iostream>
#include <vector>

#include "dsa/dsa.hpp"
#include "fmt/base.h"

int main() {
  Dsa dsa;

  std::string src = "Hello, World!";
  std::string dst(src.size(), 0);

  dsa.data_move(src.data(), dst.data(), src.size());

  fmt::println("{}", dst);

  return 0;
}
