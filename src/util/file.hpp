#ifndef __FILE_HPP__
#define __FILE_HPP__

#include <string>
#include <filesystem>
#include <unordered_map>

typedef std::unordered_map<std::string, std::string> directory_map;
void read_directory(directory_map &, std::string dir);

#endif