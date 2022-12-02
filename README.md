# Web-Server
[![CodeQL](https://github.com/jackwthake/Web-Server/actions/workflows/codeql.yml/badge.svg)](https://github.com/jackwthake/Web-Server/actions/workflows/codeql.yml)  
A small light-weight multithreaded HTTP server
## Building
```
clone https://github.com/jackwthake/Web-Server
cd Web-Server
make bin/serve
```
## Request Processing
1. New request is recieved in ```server_listen_loop()```
2. ```server_listen_loop()``` opens the request, queueing a new job to process the request
3. Free thread picks up the job
4. ```handle_connection()``` retrieves basic information about the request
5. ```handle_connection()``` attempts to query the router for a matching path
6. ```handle_connection()``` sends the appropriate data back to the client
7. ```handle_connection()``` closes out the client socket
## TO-DO
- [x] Serve HTTP requests
- [x] File logging
- [x] Automatically route files
- [x] Multithreaded handling of requests
- [x] Thread pooling
- [X] Use OpenSSL to serve HTTPS requests
- [ ] Server side JSON Parsing
