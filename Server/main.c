//
//  main.c
//  Server
//
//  Created by Jack Thake on 2/4/21.
//

#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "Util/net.h"
#include "http.h"

#define LISTENING_PORT "80"


// initialise the server, error check
static int server_init(int *listen_fd) {
    *listen_fd = get_listener_socket(LISTENING_PORT);
    if (listen_fd < 0) {
        fprintf(stderr, "webserver: fatal error getting listening socket\n");
        return -1;
    }
    
    printf("webserver: waiting for connections on port %s\n", LISTENING_PORT);
    
    return 1;
}


// listens for new connections, handles requests
static void server_listen(const int listen_fd) {
    struct sockaddr_storage their_addr; // client info
    socklen_t sin_size = sizeof their_addr;
    char s[INET6_ADDRSTRLEN];
    
    
    const int new_fd = accept(listen_fd, (struct sockaddr *)&their_addr, &sin_size); // look for new connection
    
    if (new_fd == -1) { // connection couldn't be accepted
        perror("accept");
    } else { // connection successful
        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s); // get address
        printf("webserver: connection from %s\n", s);
        
        // TODO: handle connection
        http_handle_request(new_fd);
        
        // done with connection.
        close(new_fd);
    }
}


int main(int argc, const char * argv[]) {
    int listen_fd; // server listens on listen_fd, recieves connections on new_fd
    
    // initialise server
    if (!server_init(&listen_fd))
        return -1;
    
    for(;;) // listen loop, TODO: remove infinite loop
        server_listen(listen_fd);
    
    return 0;
}
