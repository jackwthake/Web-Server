#include "server.hpp"

#include <iostream>
#include <fstream>
#include <thread>

#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/err.h>

#include "util/pool.hpp"
#include "util/log.hpp"

#define ROUTER_CONFIG_PATH "./routing.conf"
#define MAX_LINE            4096
#define BACKLOG             10
#define PORT                8000


/*****************************
 * Static helper functions
******************************/

/*
 * check C-Library functions that set errno
*/
static void error_check(int cond, const std::string msg) {
  if (cond < 0) {
    log_info("ERROR: %s: %s", msg.c_str(), strerror(errno));
    exit(EXIT_FAILURE);
  }
}


/*
 * Add response code to response
*/
static void add_response_code(std::string &response, const int code, const std::string msg) { 
  response += "HTTP/1.0 ";
  response += std::to_string(code);
  response += " " + msg + "\n";
}


/*
 * Add header to response
*/
static void add_header(std::string &response, const std::string header_key, const std::string &header_val) { 
  response += header_key + ": " + header_val;
}


/*
 * Add the body to an request
*/
static void add_body(std::string &response, const std::string &body) { 
  response += "\r\n\r\n" + body;
}


/*
 * Get info related to a request
*/
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



/*****************************
 * Request handling functions
******************************/

/**
 *  handles one get request, querying the router, building an adequate response
*/
static void handle_get_request(const https_server *server, std::string &response, std::string &path) { 
  const https_server::file_info *file = server->get_endpoint(path); // attempt to find route

  if (file != NULL) { // route found, send contents
    add_response_code(response, 200, "OK");
    add_header(response, "Content-Type", file->MIME_type);
    add_body(response, file->contents);
  } else { // no route found in config
    auto file_404 = server->get_endpoint("/404");

    add_response_code(response, 404, "NOT FOUND");
    add_header(response, "Content-Type", file_404->MIME_type);
    add_body(response, file_404->contents);
  }
}


static void handle_post_request(std::string &response, std::string &path) { 
  /* UNIMPLMENTED */
}


/**
 * Handle an incoming connection, this function will be called by one of the threads in the thread pool.
 * uses openSSL to read and write data to the client socket.
*/
static void handle_connection(job_t::info_t job_info) { 
  std::string request, response, path, method;
  char recv_buf[MAX_LINE + 1];
  int n;

  /* read in request */
  memset(recv_buf, 0x00, MAX_LINE + 1);
  while ((n = SSL_read(job_info.ssl, recv_buf, MAX_LINE)) > 0) {
    request.append(recv_buf, n);
    if (recv_buf[n - 1] == '\n') // check for end of request
      break;
    
    memset(recv_buf, 0x00, MAX_LINE);
  }

  /* Process Request */
  get_req_info(request, method, path); // get path and method
  log_info("SERVER: INCOMING CONNECTION: %12s %5s %s", inet_ntoa(job_info.client_addr.sin_addr), method.c_str(), path.c_str());

  /* build appropriate response */
  if (method.compare("GET") == 0) {
    handle_get_request(job_info.server, response, path);
  } else if (method.compare("POST") == 0) {
    handle_post_request(response, path);
  }

  /* write response back to client */
  SSL_write(job_info.ssl, response.c_str(), response.length());

  /* close connection */
  SSL_shutdown(job_info.ssl);
  SSL_free(job_info.ssl);
  close(job_info.client_fd);
}



/*************************************
 * https_server class implementation
**************************************/

https_server::https_server() { 
  this->populate_router();
  this->create_SSL_context();
  this->configure_SSL_context();

  this->socket_fd = this->create_server_socket();
  this->main_loop();
}


https_server::~https_server() { 
  close(this->socket_fd);
  close_log_file();
}


/**
 * Searches the server's internal routing hash map, returning any matches 
*/
const https_server::file_info *https_server::get_endpoint(std::string path) const {
  auto route = this->routing.find(path);
  if (route == end(this->routing)) {
    return NULL;
  }

    return &route->second;
}


/**
 * Create the server's listening socket
*/
int https_server::create_server_socket() { 
  int listen_fd;
  struct sockaddr_in servaddr;

  // create the socket
  error_check((listen_fd = socket(AF_INET, SOCK_STREAM, 0)), "Socket error");
  
  // setup address
  bzero(&servaddr, sizeof(struct sockaddr_in));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(PORT);

  // attempt to bind address to socket
  error_check(bind(listen_fd, (sockaddr *)&servaddr, sizeof(servaddr)), "Bind error");
  
  // attempt to start listening
  error_check(listen(listen_fd, BACKLOG), "Listen error");
  log_info("SERVER: Server intialised using file descriptor %d, on port %d", listen_fd, PORT);
  
  return listen_fd;
}


/**
 * Main loop of the server, waits for connections and attempts to establish a secure connection
*/
void https_server::main_loop() { 
  struct sockaddr_in client_addr;
  socklen_t client_len;

  for (;;) {
    int client_fd = accept(this->socket_fd, (struct sockaddr*)&client_addr, &client_len); /* blocks until request */

    /* set up ssl for socket */
    SSL *ssl = SSL_new(this->ssl_ctx);
    SSL_set_fd(ssl, client_fd);

    if (SSL_accept(ssl)) {
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

      this->pool.queue_job(job);
    }
  }
}


/**
 * Creates an SSL context and error checks
*/
void https_server::create_SSL_context() {
  const SSL_METHOD *method = TLS_server_method();

  this->ssl_ctx = SSL_CTX_new(method);
  if (!this->ssl_ctx) {
    error_check(-1, "Unable to create SSL Context.");
  }
}


/**
 * Load certificates and keys
*/
void https_server::configure_SSL_context() {
  error_check(SSL_CTX_use_certificate_chain_file(this->ssl_ctx, "./secret/certificate.crt"), "Failed to load certificate.");
  error_check(SSL_CTX_use_PrivateKey_file(this->ssl_ctx, "./secret/secret.key", SSL_FILETYPE_PEM), "Unable to load key file.");
}


/**
 * Seaerches the default routing config file, populating the hash map with valid routes,
 * if a route is not in the hash map, the route will not be served.
*/
void https_server::populate_router() {
  std::ifstream fp;

  // open routing config file
  fp.open(ROUTER_CONFIG_PATH);
  if (!fp) {
    std::cout << "Failed to open routing config file at path: routing.conf"  << std::endl;
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
    this->routing.insert({ route, file });
    log_info("ROUTER: Attached route %s to file path %s.", route.c_str(), file.path.c_str());
  } while (!fp.eof());
}
