#include <iostream>
#include <thread>

#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "route.hpp"
#include "../util/log.hpp"

#define PORT 8000
#define MAX_LINE 4096
#define BACKLOG 10

static Router router("./routing.conf");


void handle_connection(int client_fd, struct sockaddr_in);


/*
 * Helper function for checking C-Library functions that set errno
*/
void error_check(int cond, const std::string msg) {
  if (cond < 0) {
    log_info("ERROR: %s: %s", msg.c_str(), strerror(errno));
    exit(EXIT_FAILURE);
  }
}


/*
 * Adds response code to string
*/
void add_res_code(std::string &res, const int code, const std::string msg) {
  res += "HTTP/1.0 ";
  res += std::to_string(code);
  res += " " + msg + "\n";
}


/*
 * Appends a header to the response string
*/
void add_header(std::string &res, const std::pair<std::string, std::string> header) {
  res += header.first + ": " + header.second;
}


/*
 * Appends the body to the response
*/
void add_body(std::string &res, const std::string body) {
  res += "\r\n\r\n" + body;
}


/*
 * Get info related to a request
*/
void get_req_info(const std::string &req, std::string &method, std::string &path) {
  const char *data = req.data();
  bool found_method = false;

  /* loop through the request */
  for (int i = 0; i < strlen(data); ++i) {
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


/*
 * Initialize the server and handle errors.
*/
int server_init(void) {
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
  
  return listen_fd;
}


/*
 * Listen for new requests
*/
void server_listen_loop(int listen_fd) {
  struct sockaddr_in client_addr;
  socklen_t client_len;
  int client_fd;
  
  for (;;) {
    client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len); /* blocks until request */

    auto thread = std::thread(handle_connection, client_fd, client_addr); /* imediately spin up thread for request */
    thread.detach();
  }
}


/*
 * Handle a single request
*/
void handle_connection(int client_fd, struct sockaddr_in client_addr) {
  std::string request, response, path, method;
  char recv_buf[MAX_LINE + 1];
  int n;

  /* read in request */
  memset(recv_buf, 0x00, MAX_LINE + 1);
  while ((n = read(client_fd, recv_buf, MAX_LINE)) > 0) {
    request.append(recv_buf, n);
    if (recv_buf[n - 1] == '\n') // check for end of request
      break;
    
    memset(recv_buf, 0x00, MAX_LINE);
  }

  /* Process Request */
  get_req_info(request, method, path); // get path and method

  // print info
  log_info("INCOMING CONNECTION: %12s %5s %s", inet_ntoa(client_addr.sin_addr), method.c_str(), path.c_str());

  if (method.compare("GET") == 0) {
    const file_info *file = router.get_end_point(path); // attempt to find route

    if (file != NULL) { // route found, send contents
      add_res_code(response, 200, "OK");
      add_header(response, { "Content-Type", file->MIME_type });
      add_body(response, file->contents);
    } else { // no route found in config
      auto file_404 = router.get_end_point("/404");

      add_res_code(response, 404, "NOT FOUND");
      add_header(response, { "Content-Type", file_404->MIME_type });
      add_body(response, file_404->contents);
    }
  }

  /* write to the client */
  write(client_fd, response.c_str(), response.length());
  /* clean up file descriptors */
  close(client_fd);
}
