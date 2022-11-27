#include <iostream>
#include <thread>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "route.hpp"

#define PORT 8000
#define MAX_LINE 4096
#define BACKLOG 10

#define ERROR(msg) { \
  std::cerr << msg << strerror(errno) << std::endl; \
  exit(EXIT_FAILURE); \
}


static Router router("./routing.conf");

void process_request(int client_fd, std::string request);


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


int server_init(void) {
  int listen_fd;
  struct sockaddr_in servaddr;

  // create the socket
  if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    ERROR("Socket error. ");
  
  // setup address
  bzero(&servaddr, sizeof(struct sockaddr_in));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(PORT);

  // attempt to bind address to socket
  if (bind(listen_fd, (sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    ERROR("Bind error. ");
  
  // attempt to start listening
  if (listen(listen_fd, BACKLOG) < 0)
    ERROR("Listen error. ");
  
  return listen_fd;
}


/*
 * Listen for new requests
*/
void server_listen_loop(int listen_fd) {
  int client_fd, n;
  uint8_t recv_line[MAX_LINE + 1];
  
  for (;;) {
    std::string request;

    client_fd = accept(listen_fd, NULL, NULL);
    memset(recv_line, 0x00, MAX_LINE);

    // read in request;
    while ((n = read(client_fd, recv_line, MAX_LINE - 1)) > 0) {
      request.append((char *)recv_line, n);
      if (recv_line[n - 1] == '\n') // check for end of request
        break;
      
      memset(recv_line, 0x00, MAX_LINE);
    }

    if (n < 0) // check read
      ERROR("Read fail. ");
    
    // Process request and send data
    auto thread = std::thread(process_request, client_fd, request);
    thread.detach();
  }
}


/*
 * read the request, generate the appropriate response and send to client
*/
void process_request(int client_fd, std::string req) {
  std::string response, method, path;

  get_req_info(req, method, path);

  if (method.compare("GET") == 0) {
    const file_info *file = router.get_end_point(path); // attempt to find route

    if (file != NULL) { // route found, send contents
      add_res_code(response, 200, "OK");
      add_header(response, { "Content-Type", file->MIME_type });
      add_body(response, file->contents);
    } else { // no route found in config
      add_res_code(response, 404, "NOT FOUND");
      add_body(response, "NOT FOUND");
    }
  }

  // send data
  write(client_fd, response.c_str(), response.length());
  close(client_fd);
}