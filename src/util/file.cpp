#include "file.hpp"

void read_directory(directory_map &dir_map, string dir) {
  for (const auto &entry : filesystem::directory_iterator(dir)) {
    if (entry.is_directory()) { // recursive read other directories
      read_directory(dir_map, entry.path());
      continue;
    }
    
    // load file and add to hash map
    fstream str(entry.path(), fstream::in);
    string contents((istreambuf_iterator<char>(str)), istreambuf_iterator<char>());
    dir_map.insert({ string(entry.path()), contents });
    str.close();
  }
}