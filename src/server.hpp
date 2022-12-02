#ifndef __SERVE_HPP__
#define __SERVE_HPP__

#include <unordered_map>

#include <openssl/ssl.h>
#include <netinet/in.h> // struct sockaddr_in

#include "util/pool.hpp"

class https_server {
  public:
    struct file_info {
      std::string contents, MIME_type, path;
    };

    https_server();
    ~https_server();

    const https_server::file_info *get_endpoint(std::string path) const;
  private:
    int create_server_socket();
    void main_loop();

    void create_SSL_context();
    void configure_SSL_context();
    void populate_router();

    thread_pool pool;
    SSL_CTX *ssl_ctx;

    int socket_fd;
    std::unordered_map<std::string, file_info> routing;
};

#endif