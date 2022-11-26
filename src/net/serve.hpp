#ifndef __SERVE_HPP__
#define __SERVE_HPP__

#include <unordered_map>

#include "../util/file.hpp"

class Server {
  public:
    Server(void);
    ~Server(void);
  private:
    void listen_loop(void);
    void process_request(int client_fd, std::string &request);
    
    const std::unordered_map<std::string, std::string> MIME_types = {
      { "html", "text/html" },
      { "css", "text/css" },
      { "js", "text/javascript" },
      { "ico", "image/vnd.microsoft.icon" },
      { "png", "image/png" }
    };

    directory_map dir;
    int listen_fd;
};


#endif