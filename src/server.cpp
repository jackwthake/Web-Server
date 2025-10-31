#include "server.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <stdexcept>
#include <chrono>
#include <iomanip>

#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <arpa/inet.h>
#include <openssl/err.h>

#include "util/pool.hpp"
#include "util/log.hpp"

#define SERVER_VERSION "1.1.0"
#define MAX_LINE 4096

/*****************************
 * Static helper functions
******************************/

// Check C-Library functions that set errno
static void error_check(int cond, const std::string msg) {
  if (cond < 0) {
    std::string error_msg = msg + ": " + strerror(errno);
    log_info("ERROR: %s", error_msg.c_str());
    throw std::runtime_error(error_msg);
  }
}

// Wrapper for SSL shutdown and free
static void ssl_shutdown_wrapper(SSL *ssl) {
  if (ssl) {
    if (SSL_shutdown(ssl) == 0) {
      SSL_shutdown(ssl); // bidirectional shutdown
    }

    SSL_free(ssl);
  }
}

// Add response code to response
static void add_response_code(std::string &response, const int code, const std::string msg) { 
  response += "HTTP/1.0 ";
  response += std::to_string(code);
  response += " " + msg + "\n";
}

// Add header to response
static void add_header(std::string &response, const std::string header_key, const std::string &header_val) { 
  response += header_key + ": " + header_val;
}

// Add the body to an request
static void add_body(std::string &response, const std::string &body) { 
  response += "\r\n\r\n" + body;
}

// Get info related to a request
static void get_req_info(const std::string &req, std::string &method, std::string &path) {
  const char *data = req.data();
  bool found_method = false;

  /* loop through the request */
  for (size_t i = 0; i < strlen(data); ++i) {
    if (!found_method) {
      if (!isspace(data[i]))
        method += data[i];
      else
        found_method = true;
    } else {
      if (!isspace(data[i]))
        path += data[i];
      else
        return; /* nothing left to extract */
    }
  }
}

// Get OS information from /etc/os-release
static std::string get_os_info() {
  std::ifstream os_release("/etc/os-release");
  std::string line;

  while (std::getline(os_release, line)) {
    if (line.find("PRETTY_NAME=") == 0) {
      // Extract value between quotes
      size_t first = line.find('"');
      size_t last = line.rfind('"');
      if (first != std::string::npos && last != std::string::npos && first != last) {
        return line.substr(first + 1, last - first - 1);
      }
    }
  }
  return "Unknown";
}

// Format uptime in seconds to readable string (e.g., "2d 3h 15m 30s")
static std::string format_uptime(time_t uptime_seconds) {
  auto duration = std::chrono::seconds(uptime_seconds);

  auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
  duration -= hours;
  auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
  duration -= minutes;
  auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);

  auto days = hours.count() / 24;
  auto remaining_hours = hours.count() % 24;

  std::string result;
  if (days > 0) result += std::to_string(days) + "d ";
  if (remaining_hours > 0) result += std::to_string(remaining_hours) + "h ";
  if (minutes.count() > 0) result += std::to_string(minutes.count()) + "m ";
  result += std::to_string(seconds.count()) + "s";

  return result;
}

// Get number of rate limited requests from IP log table
static int get_rate_limited_count(const https_server::ip_log_table_t &ip_log_table, int threshold) {
  int total_count = 0;
  for (const auto &entry : ip_log_table) {
    int entry_req_count = entry.second.first.load();
    total_count += entry_req_count >= threshold ? entry_req_count - threshold : 0;
  }

  return total_count;
}

// Cull entries from the IP log table that are older than the threshold
static void cull_ip_log_table(https_server::ip_log_table_t &ip_log_table, time_t current_time, time_t threshold) {
  for (auto it = ip_log_table.begin(); it != ip_log_table.end(); ) {
    time_t last_request_time = it->second.second.load();
    if (current_time - last_request_time > threshold) {
      it = ip_log_table.erase(it);
    } else {
      ++it;
    }
  }
}

// Log the IP log table to a CSV file
static void log_ip_table_csv(const https_server::ip_log_table_t &ip_log_table, const std::string &log_path) {
  log_info("SERVER: Writing IP log table to CSV at path: %s with %zu entries", log_path.c_str(), ip_log_table.size());

  std::ofstream log_file(log_path, std::ios::out | std::ios::trunc);
  if (!log_file.is_open()) {
    log_info("ERROR: Unable to open IP log file at path: %s", log_path.c_str());
    return;
  }

  log_file << "IP Address,Request Count,Last Request Timestamp,Last Request Time\n";
  for (const auto &entry : ip_log_table) {
    time_t timestamp = entry.second.second.load();
    char time_buffer[32];
    std::strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&timestamp));

    log_file << entry.first << ","
             << entry.second.first.load() << ","
             << timestamp << ","
             << time_buffer << "\n";
  }

  log_file.close();
  log_info("SERVER: Successfully wrote IP log CSV to %s", log_path.c_str());
}

/*****************************
 * Request handling functions
******************************/

// Check if an IP address is rate limited based on the log table
static bool is_rate_limited(https_server::ip_log_table_t &ip_log_table, const std::string &ip_address, unsigned long max_requests, time_t time_window) {
  time_t current_time = time(nullptr);
  auto &entry = ip_log_table[ip_address];

  std::atomic<unsigned long> &request_count = entry.first;
  std::atomic<time_t> &last_request_time = entry.second;

  // Atomically check and reset if outside time window
  time_t last_time = last_request_time.load();
  if (current_time - last_time > time_window) {
    // Attempt to atomically update the timestamp
    if (last_request_time.compare_exchange_strong(last_time, current_time)) {
      // We won the race - reset the counter to 1 (this request)
      request_count.store(1);
      return false; // Not rate limited on reset
    }
    // Someone else reset it, fall through to increment
  }

  // Increment and check if over limit
  unsigned long new_count = request_count.fetch_add(1) + 1;
  return new_count > max_requests;
}

// Handle the /status endpoint, returning server statistics in JSON format
void handle_status_endpoint(const https_server *server, std::string &response, struct in_addr &client_addr) {
  // Get system info
  struct utsname sys_info;
  uname(&sys_info);
  std::string os_name = get_os_info();

  time_t uptime_seconds = time(nullptr) - server->start_time;
  std::string uptime_str = format_uptime(uptime_seconds);

  std::string body = "{\n";
  body +=            "  \"uptime\": \"" + uptime_str + "\",\n";
  body +=            "  \"platform\": \"" + std::string(sys_info.sysname) + "\",\n";
  body +=            "  \"os_version\": \"" + os_name + "\",\n";
  body +=            "  \"server_version\": \"" + std::string(SERVER_VERSION) + "\",\n";
  body +=            "  \"thread_count\": " + std::to_string(server->get_thread_count()) + ",\n";
  body +=            "  \"total_requests\": " + std::to_string(server->total_requests) + ",\n";
  body +=            "  \"valid_requests\": " + std::to_string(server->valid_request_count) + ",\n";
  body +=            "  \"successful_requests\": " + std::to_string(server->successful_request_count) + ",\n";
  body +=            "  \"rate_limited_requests\": " + std::to_string(get_rate_limited_count(server->ip_log_table, std::get<int>(server->get_config_value("rate_limit_max_requests", 100)))) + "\n";
  body +=            "}\n";

  add_response_code(response, 200, "OK");
  add_header(response, "Content-Type", "application/json");
  add_body(response, body);

  // Count this as valid and successful (200 OK)
  server->valid_request_count++;
  server->successful_request_count++;

  log_info("SERVER: INCOMING CONNECTION: %12s GET /status -> 200 OK", inet_ntoa(client_addr));
}

//  handles one get request, querying the router, building an adequate response
static void handle_get_request(const https_server *server, std::string &response, std::string &path, struct in_addr &client_addr) {
  if (path.compare("/status") == 0) {
    handle_status_endpoint(server, response, client_addr);
    return;
  }

  auto file = server->get_endpoint(path); // attempt to find route

  if (file.has_value()) { // route found, send contents
    const auto& file_info = file->get();
    add_response_code(response, 200, "OK");
    add_header(response, "Content-Type", file_info.MIME_type);
    add_body(response, file_info.contents);

    // Count this as valid and successful (200 OK)
    server->valid_request_count++;
    server->successful_request_count++;

    log_info("SERVER: INCOMING CONNECTION: %12s GET %s -> 200 OK", inet_ntoa(client_addr), path.c_str());
  } else { // no route found in config
    auto file_404 = server->get_endpoint("/404");

    if (file_404.has_value()) {
      const auto& file_404_info = file_404->get();
      add_response_code(response, 404, "NOT FOUND");
      add_header(response, "Content-Type", file_404_info.MIME_type);
      add_body(response, file_404_info.contents);
    } else {
      // Fallback if /404 route doesn't exist
      add_response_code(response, 404, "NOT FOUND");
      add_header(response, "Content-Type", "text/plain");
      add_body(response, "404 - Page Not Found");
    }

    // Count this as valid but not successful (404)
    server->valid_request_count++;

    log_info("SERVER: INCOMING CONNECTION: %12s GET %s -> 404 ERR NOT FOUND", inet_ntoa(client_addr), path.c_str());
  }
}

// Handle an incoming connection, this function will be called by one of the threads in the thread pool.
// uses openSSL to read and write data to the client socket.
static void handle_connection(job_t::info_t job_info) {
  std::string request, response, path, method;
  char recv_buf[MAX_LINE + 1];
  int n, recv_bytes = 0;

  /* read in request */
  memset(recv_buf, 0x00, MAX_LINE + 1);
  while ((n = SSL_read(job_info.ssl, recv_buf, MAX_LINE)) > 0) {
    request.append(recv_buf, n);
    recv_bytes += n;

    if (recv_buf[n - 1] == '\n') // check for end of request
      break;

    memset(recv_buf, 0x00, MAX_LINE);
  }

  if (recv_bytes <= 0) {
    log_info("SERVER: INCOMING CONNECTION: %12s - Empty or malformed request received. dropping connection.", inet_ntoa(job_info.client_addr.sin_addr));
    ssl_shutdown_wrapper(job_info.ssl);
    close(job_info.client_fd);

    return;
  }

  /* Process Request */
  get_req_info(request, method, path); // get path and method

  /* build appropriate response */
  if (method.compare("GET") == 0) {
    handle_get_request(job_info.server, response, path, job_info.client_addr.sin_addr);
  } else {
    // Method not allowed for static site
    add_response_code(response, 405, "METHOD NOT ALLOWED");
    add_header(response, "Content-Type", "text/plain");
    add_header(response, "Allow", "GET");
    add_body(response, "405 - Method Not Allowed");

    // 405 is a valid response to a malformed/unsupported request
    job_info.server->valid_request_count++;

    const char* log_method = method.empty() ? "<empty>" : method.c_str();
    const char* log_path = path.empty() ? "<empty>" : path.c_str();
    log_info("SERVER: INCOMING CONNECTION: %12s %s %s -> 405 ERR METHOD NOT ALLOWED",
             inet_ntoa(job_info.client_addr.sin_addr), log_method, log_path);
  }

  /* write response back to client */
  int bytes = SSL_write(job_info.ssl, response.c_str(), response.length());
  if (bytes <= 0) {
    log_info("SERVER: ERROR: Failed to send response to client %s, %s", inet_ntoa(job_info.client_addr.sin_addr), ERR_error_string(ERR_get_error(), nullptr));
  }

  /* close connection */
  ssl_shutdown_wrapper(job_info.ssl);
  close(job_info.client_fd);
}


/*************************************
 * https_server class implementation
**************************************/

https_server::https_server() : start_time(time(nullptr)) {
  this->populate_config();

  // Create thread pool with configured size
  int thread_count = std::get<int>(this->get_config_value("thread_pool_size", 0));
  this->pool = std::make_unique<thread_pool>(thread_count);

  this->populate_router();
  this->create_SSL_context();
  this->configure_SSL_context();

  this->socket_fd = this->create_server_socket();
  this->main_loop();
}

https_server::~https_server() {
  log_info("SERVER: Cleaning up resources and closing connections");
  close(this->socket_fd);
  close_log_file();
}

// Searches the server's internal routing hash map, returning any matches
std::optional<std::reference_wrapper<const https_server::file_info>> https_server::get_endpoint(const std::string &path) const {
  auto route = this->routing.find(path);
  if (route == end(this->routing)) {
    return std::nullopt;
  }

  return std::cref(route->second);
}

// Create the server's listening socket
int https_server::create_server_socket() {
  int listen_fd;
  struct sockaddr_in servaddr;

  // Get config values
  int port = std::get<int>(this->get_config_value("server_port", 443));
  int backlog = std::get<int>(this->get_config_value("backlog", 1000));

  // create the socket
  error_check((listen_fd = socket(AF_INET, SOCK_STREAM, 0)), "Socket error");

  // setup address
  bzero(&servaddr, sizeof(struct sockaddr_in));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(port);

  // attempt to bind address to socket
  error_check(bind(listen_fd, (sockaddr *)&servaddr, sizeof(servaddr)), "Bind error");

  // attempt to start listening
  error_check(listen(listen_fd, backlog), "Listen error");
  log_info("SERVER: Server intialised using file descriptor %d, on port %d", listen_fd, port);

  return listen_fd;
}

// Main loop of the server, waits for connections and attempts to establish a secure connection
void https_server::main_loop() {
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);

  int last_culled = 0;

  for (;;) {
    // Accept incoming connections
    int client_fd = accept(this->socket_fd, (struct sockaddr*)&client_addr, &client_len); /* blocks until request */
    this->total_requests++; // increment total requests on every connection attempt

    // Cull log file every 100 requests if it exceeds max size
    if (this->total_requests.load() - last_culled >= 100) {
      cull_log_file(std::get<int>(this->get_config_value("log_max_size", 52428800))); // default 50MB
      cull_log_file(std::get<int>(this->get_config_value("log_max_size", 52428800)), "../logs/reboot.log");

      cull_ip_log_table(this->ip_log_table, time(nullptr), std::get<int>(this->get_config_value("ip_log_cull_threshold", 3600))); // default 1 hour
      log_ip_table_csv(this->ip_log_table, "../logs/ip_log.csv");

      last_culled = this->total_requests.load();
    }

    // Check rate limiting (this also increments the IP table counter)
    if (is_rate_limited(this->ip_log_table, inet_ntoa(client_addr.sin_addr),
                        std::get<int>(this->get_config_value("rate_limit_max_requests", 100)),
                        std::get<int>(this->get_config_value("rate_limit_time_window", 60)))) {
      log_info("SERVER: INCOMING CONNECTION: %12s - Rate limit exceeded, dropping connection.", inet_ntoa(client_addr.sin_addr));
      close(client_fd);
      continue;
    }

    if (client_fd < 0) {
      log_info("SERVER: ERROR: Accept failed: %s", strerror(errno));
      continue;
    }

    /* set up ssl for socket */
    SSL *ssl = SSL_new(this->ssl_ctx.get());
    if (!ssl) {
      log_info("SERVER: ERROR: Unable to create SSL structure for client %s, dropping connection.", inet_ntoa(client_addr.sin_addr));
      close(client_fd);
      continue;
    }
    
    SSL_set_fd(ssl, client_fd);

    int accept_result = SSL_accept(ssl);
    if (accept_result == 1) {
      /* submit job */
      job_t job = {
        {
          this,
          client_addr,
          ssl,
          client_fd
        },
        handle_connection
      };

      this->pool->queue_job(job);
    } else {
      /* SSL handshake failed, clean up resources */
      int ssl_error = SSL_get_error(ssl, accept_result);
      unsigned long err_code = ERR_get_error();
      char err_buf[256];
      ERR_error_string_n(err_code, err_buf, sizeof(err_buf));

      log_info("SERVER: SSL handshake failed for client %s - SSL_error: %d, Error: %s",
               inet_ntoa(client_addr.sin_addr), ssl_error, err_buf);

      SSL_free(ssl);
      close(client_fd);
    }
  }
}

// Creates an SSL context and error checks
void https_server::create_SSL_context() {
  const SSL_METHOD *method = TLS_server_method();

  SSL_CTX* ctx = SSL_CTX_new(method);
  if (!ctx) {
    throw std::runtime_error("Unable to create SSL Context.");
  }
  this->ssl_ctx.reset(ctx);
}

// Load certificates and keys
void https_server::configure_SSL_context() {
  error_check(SSL_CTX_use_certificate_chain_file(this->ssl_ctx.get(), std::get<std::string>(this->get_config_value("ssl_cert_path", "./secret/server.crt")).c_str()), "Failed to load certificate.");
  error_check(SSL_CTX_use_PrivateKey_file(this->ssl_ctx.get(), std::get<std::string>(this->get_config_value("ssl_key_path", "./secret/server.key")).c_str(), SSL_FILETYPE_PEM), "Unable to load key file.");
}

// Searches the default routing config file, populating the hash map with valid routes,
// if a route is not in the hash map, the route will not be served.
void https_server::populate_router() {
  std::ifstream fp;

  // open routing config file
  std::string router_path = std::get<std::string>(this->get_config_value("router_config_path", "./public/endpoints.conf"));
  fp.open(router_path);
  if (!fp) {
    throw std::runtime_error("Failed to open routing config file at path: " + router_path);
  }

  // read each route into memory
  std::string line;
  while (std::getline(fp, line)) {
    // Skip empty lines and lines with only whitespace
    if (line.empty() || line.find_first_not_of(" \t\r\n") == std::string::npos) {
      continue;
    }

    // Skip comment lines (lines starting with #)
    size_t first_char = line.find_first_not_of(" \t");
    if (first_char != std::string::npos && line[first_char] == '#') {
      continue;
    }

    // Parse the line: route path mime-type
    std::istringstream iss(line);
    std::string route;
    file_info file;

    if (!(iss >> route >> file.path >> file.MIME_type)) {
      log_info("ERROR: Invalid routing.conf format on line: %s", line.c_str());
      continue;
    }

    // load file content
    std::fstream str(file.path, std::fstream::in);
    if (!str) {
      log_info("ERROR: Cannot open route file: %s - skipping route %s", file.path.c_str(), route.c_str());
      continue;  // Skip this route but continue processing others
    }

    file.contents = std::string((std::istreambuf_iterator<char>(str)), std::istreambuf_iterator<char>());

    // insert route into table
    this->routing.insert({ route, file });
    log_info("ROUTER: Attached route %s to file path %s.", route.c_str(), file.path.c_str());
  }
}

// Implementation for populating server configuration from a file or defaults
void https_server::populate_config() {
  std::ifstream fp;

  // open config file
  fp.open("./secure-serve.conf");
  if (!fp) {
    log_info("CONFIG: No config file found at ./secure-serve.conf");
    return;
  }

  // read each config line
  std::string line;
  while (std::getline(fp, line)) {
    // Skip empty lines and lines with only whitespace
    if (line.empty() || line.find_first_not_of(" \t\r\n") == std::string::npos) {
      continue;
    }

    // Skip comment lines (lines starting with #)
    size_t first_char = line.find_first_not_of(" \t");
    if (first_char != std::string::npos && line[first_char] == '#') {
      continue;
    }

    // Parse the line: key=value
    std::istringstream iss(line);
    std::string key, value_str;

    if (!std::getline(iss, key, '=') || !std::getline(iss, value_str)) {
      log_info("CONFIG: Invalid format on line: %s", line.c_str());
      continue;
    }

    // Trim whitespace from key and value
    key.erase(0, key.find_first_not_of(" \t"));
    key.erase(key.find_last_not_of(" \t") + 1);
    value_str.erase(0, value_str.find_first_not_of(" \t"));
    value_str.erase(value_str.find_last_not_of(" \t") + 1);

    // Try to parse as int, otherwise store as string
    try {
      int int_value = std::stoi(value_str);
      this->config[key] = int_value;
    } catch (const std::exception&) {
      // Not an int, store as string
      this->config[key] = value_str;
    }
  }
}

// Helper to get config value with default
https_server::config_value_t https_server::get_config_value(const std::string &key, const config_value_t &default_value) const {
  auto it = this->config.find(key);
  
  if (it != this->config.end()) {
    return it->second;
  }

  return default_value;
}

