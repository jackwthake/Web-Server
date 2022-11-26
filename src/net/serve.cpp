#include <iostream>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "serve.hpp"

#define PORT 8000
#define MAX_LINE 4096
#define BACKLOG 10


#define ERROR(msg) { \
  std::cerr << msg << strerror(errno) << std::endl; \
  exit(EXIT_FAILURE); \
}

/*
 * Adds response code to string
*/
static void add_res_code(std::string &res, const int code, const std::string msg) {
  res += "HTTP/1.0 ";
  res += std::to_string(code);
  res += " " + msg + "\r\n\r\n";
}


/*
 * Appends a header to the response string
*/
static void add_header(std::string &res, const std::pair<std::string, std::string> header) {
  res += header.first + ": " + header.second + "\r\n\r\n";
}


Server::Server(void) {
  struct sockaddr_in servaddr;

  // create the socket
  if ((this->listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
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
  if (listen(this->listen_fd, BACKLOG) < 0)
    ERROR("Listen error. ");
  
  // start listening loop:
  this->listen_loop();
}


Server::~Server(void) {
  close(this->listen_fd);
}


void Server::listen_loop(void) {
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
    this->process_request(client_fd, request);
    
    close(client_fd);
  }
}


void Server::process_request(int client_fd, std::string &req) {
  std::string response;

  // need to get the request type
  // need to get the path
  // process path
  // load the appropriate file, etc
  // send resposne

  add_res_code(response, 200, "OK");
  response += "Hello, World!";
  write(client_fd, response.c_str(), response.length());
}