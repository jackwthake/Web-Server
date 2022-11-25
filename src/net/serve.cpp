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
  uint8_t buff[MAX_LINE + 1], recv_line[MAX_LINE + 1];

  
  for (;;) {
    client_fd = accept(listen_fd, NULL, NULL);
    memset(recv_line, 0x00, MAX_LINE);

    // read in request;
    while ((n = read(client_fd, recv_line, MAX_LINE - 1)) > 0) {
      std::cout << recv_line << std::endl;
      if (recv_line[n - 1] == '\n')
        break;
      
      memset(recv_line, 0x00, MAX_LINE);
    }

    if (n < 0) // check read
      ERROR("Read fail. ");
    
    // send back information
    snprintf((char *)buff, sizeof(buff), "HTTP/1.0 200 OK\r\n\r\nHello, World!");
    write(client_fd, (char *)buff, strnlen((char *)buff, MAX_LINE));

    close(client_fd);
  }
}
