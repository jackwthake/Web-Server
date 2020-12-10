#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "util/file.h"
#include "mime.h"

#define MAX_METHOD_LENGTH 8      // enough for 7 characters
#define MAX_PATH_LENGTH   65     // enough for 64 characters
#define MAX_REQUEST_SIZE  65536  // 64k of space
#define MAX_RESPONSE_SIZE 262144 // 256k of space

#define SERVER_ROOT "./serverroot"


/*
 * Constructs a valid HTTP response.
*/
static int send_response(int fd, char *header, char *content_type, void *body, int content_length) {
  char response[MAX_RESPONSE_SIZE];

  // date and time variables
  time_t curr_time;
  char *date_str = NULL;

  // get current time.
  time(&curr_time);
  date_str = ctime(&curr_time);

  // Construct response.
  sprintf(response, "%s\n"
                   "Host: localhost\n"
                   "Date: %s"
                   "Content-Type: %s\n"
                   "Content-Length: %d\n"
                   "Connection: close\n\r\n\r"
                   "%s", header, date_str, content_type, content_length, (char*)body);
  // send created response
  int rv = send(fd, response, strlen(response), 0);

  // error check
  if (rv < 0) {
    perror("send");
  }

  return rv;
}


/*
 * Grabs the 404 page, sends it over the network.
*/
static int send_404(const int fd) {
  char fp[4096];
  struct file_data *err_page = NULL;

  snprintf(fp, 4096, "%s/404.html", SERVER_ROOT);
  err_page = file_load(fp);
  if (!err_page) {
    perror("404 Fetch");
    exit(-3);
  }

  int bytes_sent = send_response(fd, "HTTP/1.1 404 NOT FOUND", get_mime_type(fp), err_page->data, err_page->size);

  file_free(err_page);
  return bytes_sent;
}


/*
 * Handles a request.
*/
void handle_http_request(const int fd) {
  char req[MAX_REQUEST_SIZE]; // We won't use all of this space but this is the max it could be.
  char method[MAX_METHOD_LENGTH]; // holds the method used byt the request ie. GET or POST
  char path[MAX_PATH_LENGTH]; // holds the path of the request

  size_t bytes_recieved = recv(fd, req, MAX_REQUEST_SIZE - 1, 0); // recieve the actual data

  // error check
  if (bytes_recieved < 0) {
    perror("recieve");
    return;
  }

  // parse request
  sscanf(req, "%s %s HTTP/1\n", method, path);

  // attempt to load file
  char fp[4096];

  snprintf(fp, 4096, "%s%s", SERVER_ROOT, path);
  struct file_data *req_data = file_load(fp);
  
  if (!req_data)
    send_404(fd);
  else {
    send_response(fd, "HTTP/1.1 200 OK", get_mime_type(path), req_data->data, req_data->size);
  }
}

