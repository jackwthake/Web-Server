#ifndef __ROUTE_HPP__
#define __ROUTE_HPP__

#include <string>
#include <unordered_map>

class Router {
  public:
    Router(std::string directory_path);

    struct Public_file {
      std::string contents, MIME_type, path;
    };

    const Router::Public_file *get_end_point(std::string &path);
  private:
    std::unordered_map<std::string, Router::Public_file> routes;
};

#endif