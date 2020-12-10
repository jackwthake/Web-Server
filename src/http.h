#ifndef _HTTP_H_
#define _HTTP_H_

/*
 * this holds the one exposed function for the http module.
 * the http module contains everything needed to respond to
 * a http request.
*/
extern void handle_http_request(const int fd);

#endif
