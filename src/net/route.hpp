#ifndef __ROUTE_HPP__
#define __ROUTE_HPP__

#include <string>
#include <unordered_map>

struct file_info {
  std::string contents, MIME_type, path;
};

class Router {
  public:
    Router(std::string directory_path);

    const struct file_info *get_end_point(std::string &path);
  private:
    std::unordered_map<std::string, file_info> routes;
};

#endif