#include <iostream>

#include "const.hpp"
#include "util/file.hpp"

auto main(int argc, char **argv) -> int {
  directory_map map;
  read_directory(map, "./src/");

  auto pair = map.find("./src/util/file.cpp");
  cout << pair->first << "\n\n" << pair->second << endl;

  for (auto p : map) {
    cout << p.first << endl;
  }

  return EXIT_SUCCESS;
}