#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>

#define MAX_METHOD_LENGTH 8 // enough for 7 characters
#define MAX_PATH_LENGTH 65 // enough for 64 characters
#define MAX_REQUEST_SIZE 65536 // 64k of space
#define MAX_RESPONSE_SIZE 262144


/*
 * Constructs a valid HTTP response.
*/
int send_response(int fd, char *header, char *content_type, void *body, int content_length) {
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
  printf("%s requested with method %s\n", path, method);
  send_response(fd, "HTTP/1.1 200 OK", "text/text-plain", "Tester", 7);
}

