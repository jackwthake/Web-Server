#ifndef __SERVE_HPP__
#define __SERVE_HPP__

int server_init(void);
void server_listen_loop(int socket_fd);

#endif