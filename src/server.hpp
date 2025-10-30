#ifndef __SERVE_HPP__
#define __SERVE_HPP__

#include <string>
#include <unordered_map>
#include <memory>
#include <optional>
#include <atomic>

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

    std::optional<std::reference_wrapper<const file_info>> get_endpoint(const std::string &path) const;
    size_t get_thread_count() const { return pool.get_thread_count(); }

    const time_t start_time;
    mutable std::atomic<unsigned long> total_requests{0};
    mutable std::atomic<unsigned long> valid_request_count{0};
    mutable std::atomic<unsigned long> successful_request_count{0};
  private:
    int create_server_socket();
    void main_loop();

    void create_SSL_context();
    void configure_SSL_context();
    void populate_router();

    thread_pool pool;

    // Custom deleter for SSL_CTX
    struct SSL_CTX_Deleter {
      void operator()(SSL_CTX* ctx) const {
        if (ctx) SSL_CTX_free(ctx);
      }
    };
    std::unique_ptr<SSL_CTX, SSL_CTX_Deleter> ssl_ctx;

    int socket_fd;
    std::unordered_map<std::string, file_info> routing;
};

#endif