//
//  http.c
//  Server
//
//  Created by Jack Thake on 2/4/21.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "http.h"
#include "Util/file.h"

#define MAX_METHOD_LENGTH 8      // enough for 7 characters
#define MAX_PATH_LENGTH   65     // enough for 64 characters
#define MAX_REQUEST_SIZE  65536  // 64k of space
#define MAX_RESPONSE_SIZE 262144 // 256k of space

#define SERVER_ROOT "./public"

static int http_send_response(const int fd, char *header, char *content_type, char *body, int content_length) {
    char response[MAX_RESPONSE_SIZE];
    int rv = 0;
    
    // date and time headers
    time_t curr_time;
    char *date_str = NULL;
    
    // get current time
    time(&curr_time);
    date_str = ctime(&curr_time);
    
    // construct resposne
    sprintf(response, "%s\n"
                     "Host: localhost\n"
                     "Date: %s"
                     "Content-Type: %s\n"
                     "Content-Length: %d\n"
                     "Connection: close\n\r\n\r"
                     "%s", header, date_str, content_type, content_length, body);
    
    // send response and error check
    if ((rv = send(fd, response, strlen(response), 0) < 0)) {
        perror("send");
    }
    
    return rv;
}


static void http_send_404(const int fd) {
    http_send_response(fd, "HTTP/1.1 404 NOT FOUND", "text/plain", "Not Found", 12);
}


void http_handle_request(const int fd) {
    // We allocate to the largest possible size.
    char req[MAX_REQUEST_SIZE];
    char method[MAX_METHOD_LENGTH];
    char path[MAX_PATH_LENGTH];
    char fp[4096];
    
    size_t bytes_recieved = recv(fd, req, MAX_REQUEST_SIZE - 1, 0);
    if (bytes_recieved < 0) {
        perror("recieve");
        return;
    }
    
    // parse request
    sscanf(req, "%s %s HTTP/1\n", method, path);
    
    // grab path and try loading
    snprintf(fp, 4096, "%s%s", SERVER_ROOT, path);
    struct file_data *req_data = file_load(fp);
    
    if (!req_data) {
        http_send_404(fd);
        return;
    }
    
    // send appropriate response
    http_send_response(fd, "HTTP/1.1 200 OK", "text/html", req_data->data, req_data->size);
    
    // we no longer need the file data
    file_free(req_data);
}
