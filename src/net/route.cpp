#include "route.hpp"

#include <iostream>
#include <fstream>

#define MAX_LINE 101

Router::Router(std::string directory_path) {
  std::ifstream fp;

  // open routing config file
  fp.open(directory_path);
  if (!fp) {
    std::cout << "Failed to open routing config file at path: " << directory_path << std::endl;
    exit(EXIT_FAILURE);
  }

  // read each route into memory
  do {
    file_info file;
    std::string route;
    
    // each route is stored in the config: "request_path file_path MIME_type"
    std::getline(fp, route, ' ');
    std::getline(fp, file.path, ' ');
    std::getline(fp, file.MIME_type, '\n');
    
    // load file content
    std::fstream str(file.path, std::fstream::in);
    file.contents = std::string((std::istreambuf_iterator<char>(str)), std::istreambuf_iterator<char>());

    // insert route into table
    this->routes.insert({ route, file });
  } while (!fp.eof());
}


/*
 * Returns a pointer to a route's relavent file if it exists, if it does not exist return NULL
*/
const file_info *Router::get_end_point(std::string &path) {
  auto route = this->routes.find(path);
  if (route == end(this->routes))
    return NULL;

    return &route->second;
}