# Multithreaded HTTPS Web Server

[![CodeQL](https://github.com/jackwthake/Web-Server/actions/workflows/codeql.yml/badge.svg)](https://github.com/jackwthake/Web-Server/actions/workflows/codeql.yml)

A high-performance, lightweight HTTPS web server implemented in modern C++17. Features secure TLS/SSL encryption, concurrent request handling via thread pooling, and efficient resource management using smart pointers and RAII principles.

## Overview

This project demonstrates advanced C++ systems programming concepts including network programming, multi-threading, SSL/TLS cryptography, and modern memory management techniques. Built from the ground up using POSIX sockets and OpenSSL, it showcases low-level understanding of network protocols and concurrent programming patterns.

## Key Features

### Security
- **TLS/SSL Encryption**: Full HTTPS support using OpenSSL 3.0+
- **Whitelist Routing**: Pre-loaded route table prevents path traversal attacks
- **Secure Certificate Handling**: Proper key management with automatic resource cleanup
- **Exception-Based Error Handling**: Proper cleanup and resource management during failures

### Performance
- **Thread Pool Architecture**: Dynamic worker thread pool scaling with hardware concurrency
- **Non-blocking I/O**: Efficient request handling using condition variables and mutexes
- **Resource Management**: Smart pointers with custom deleters for zero-leak guarantee

## Architecture

### Request Processing Flow

```
1. Startup
   ├─> Routes loaded from routing.conf into memory
   ├─> SSL context initialized with certificates
   └─> Thread pool created matching CPU core count

2. Client Connection
   ├─> TCP socket accepted on port 443 (HTTPS)
   └─> SSL/TLS handshake performed

3. Request Handling
   ├─> Job queued to thread pool
   ├─> Available worker thread picks up job
   └─> Request parsed and routed

4. Response Generation
   ├─> Route matched against pre-loaded routing table
   ├─> File content retrieved from memory and MIME type set
   └─> HTTP response constructed

5. Cleanup
   ├─> Response sent over SSL connection
   ├─> SSL session terminated (SSL_shutdown)
   └─> Resources automatically freed (RAII)
```

### Core Components

- **`https_server`**: Main server class managing SSL context, socket lifecycle, and routing
- **`thread_pool`**: Worker thread manager with condition variable synchronization
- **`job_t`**: Request job structure passed to worker threads
- **Routing System**: Hash-map based URL-to-file routing with pre-loaded content for security

## Building & Running

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake libssl-dev

# Fedora/RHEL
sudo dnf install gcc-c++ cmake openssl-devel
```

### Build

```bash
git clone https://github.com/jackwthake/Web-Server
cd Web-Server

# Modern CMake approach
cmake -B build
cmake --build build
cd build
sudo ./serve
```

The server will start on `https://localhost:443` (requires root/sudo for port 443)


### Testing HTTPS Connection

```bash
# Using curl (accept self-signed certificate)
curl -k https://localhost

# Using browser
# Navigate to https://localhost
# Accept security warning for self-signed certificate
```

## Configuration

### Routing Configuration

Routes are defined in `routing.conf` with the format:
```
<url_path> <file_path> <mime_type>
```

**Security Note**: All files are loaded into memory at startup, creating a whitelist of allowed routes. This prevents path traversal attacks since requests are only matched against pre-loaded routes, never accessing the filesystem dynamically.

Example:
```
/ ./public/index.html text/html
/404 ./public/404.html text/html
/css/style.css ./public/css/style.css text/css
/js/app.js ./public/js/app.js application/javascript
```

### SSL Certificates

Development certificates should be placed in `./secret/`:
- `server.crt` - SSL certificate
- `server.key` - Private key (permissions: 400)

## Technical Highlights

### Modern C++ Practices

- **Smart Pointers**: Custom deleters for OpenSSL resources (`SSL_CTX`, `SSL`)
- **Optional Types**: `std::optional` for safe null handling instead of raw pointers
- **Move Semantics**: Efficient resource transfer in thread pool
- **Type Safety**: `nullptr` instead of `NULL`, strong typing throughout

### Concurrency

- **Thread-Safe**: Mutex-protected job queue with condition variables
- **Deadlock-Free**: Scoped locking patterns with RAII
- **Scalable**: Thread pool size dynamically matches CPU core count

### Memory Management

- **Zero-Copy**: Direct SSL buffer handling
- **No Memory Leaks**: Verified with Valgrind and static analysis
- **Exception-Safe**: RAII ensures cleanup even during error conditions

## Development & Testing

### Logging

Server logs are written to `server.log` with timestamps:
```
[10/25/25 01:42:30]: SERVER: Server initialised using file descriptor 3, on port 443
[10/25/25 01:42:35]: SERVER: INCOMING CONNECTION: 127.0.0.1   GET /
[10/25/25 01:42:40]: ROUTER: Attached route / to file path ./public/index.html
```

## License

This project is provided as-is for educational and portfolio purposes.
