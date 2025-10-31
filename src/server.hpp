#ifndef __SERVE_HPP__
#define __SERVE_HPP__

#include <string>
#include <unordered_map>
#include <memory>
#include <optional>
#include <variant>
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
    size_t get_thread_count() const { return pool->get_thread_count(); }

    // Stats
    const time_t start_time;
    mutable std::atomic<unsigned long> total_requests{0};
    mutable std::atomic<unsigned long> valid_request_count{0};
    mutable std::atomic<unsigned long> successful_request_count{0};

    // ip logging and rate limiting table
    // key: ip address (string), value: request count (unsigned long)
    // value: pair<request count, last request time> atomic
    using ip_log_table_t = std::unordered_map<std::string, std::pair<std::atomic<unsigned long>, std::atomic<time_t>>>;
    // config file supports string and int types for variable values
    using config_value_t = std::variant<std::string, int>;
    config_value_t get_config_value(const std::string &key, const config_value_t &default_value) const;
  private:
    // Custom deleter for SSL_CTX
    struct SSL_CTX_Deleter {
      void operator()(SSL_CTX* ctx) const {
        if (ctx) SSL_CTX_free(ctx);
      }
    };

    int create_server_socket();
    void main_loop();

    void create_SSL_context();
    void configure_SSL_context();
    void populate_router();
    void populate_config();

    int socket_fd;

    std::unique_ptr<SSL_CTX, SSL_CTX_Deleter> ssl_ctx;
    std::unique_ptr<thread_pool> pool;

    std::unordered_map<std::string, file_info> routing;
    std::unordered_map<std::string, config_value_t> config;
    ip_log_table_t ip_log_table;

    friend void handle_status_endpoint(const https_server *server, std::string &response, struct in_addr &client_addr);
};

#endif