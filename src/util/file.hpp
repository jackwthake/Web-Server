#ifndef __FILE_HPP__
#define __FILE_HPP__

#include <fstream>
#include <string>
#include <filesystem>
#include <unordered_map>

using namespace std;

typedef unordered_map<string, string> directory_map;
void read_directory(directory_map &, string dir);

#endif