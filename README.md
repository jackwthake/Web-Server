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

## Deployment

### Automated EC2 Deployment Pipeline

This project includes a fully automated deployment pipeline for AWS EC2 instances with zero-downtime updates and automatic crash recovery.

#### Deployment Workflow

```
1. Initial Launch (EC2 User Data runs install.sh)
   ├─> Install system dependencies and certbot
   ├─> Clone repository to ~/secure-serve
   ├─> Obtain Let's Encrypt SSL certificates (jackthake.com)
   ├─> Configure systemd services (server, reboot, cert renewal)
   └─> Build and start server

2. Production Updates (git push + EC2 reboot)
   ├─> Developer pushes code to GitHub
   ├─> Reboot EC2 instance
   ├─> secure-serve-reboot.service triggers
   ├─> System updates (yum update)
   ├─> Git pulls latest code
   ├─> CMake rebuilds project
   └─> Server restarts with new version

3. Crash Recovery (automatic)
   ├─> Server process crashes/exits
   ├─> secure-serve.service detects failure
   ├─> Waits 10 seconds
   └─> Automatically restarts server

4. SSL Certificate Renewal (automatic)
   ├─> certbot-renew.timer triggers (twice daily)
   ├─> Checks if certificates need renewal (< 30 days)
   ├─> If needed: stops server, renews certs, restarts server
   └─> Logs renewal attempt
```

#### Service Management

```bash
# Check server status
sudo systemctl status secure-serve.service

# View real-time server logs
sudo journalctl -u secure-serve.service -f

# View deployment logs
sudo journalctl -u secure-serve-reboot.service

# Manual server restart
sudo systemctl restart secure-serve.service

# Manual deployment (without reboot)
sudo systemctl start secure-serve-reboot.service
```

#### Benefits

- **Zero-touch deployments**: Push code and reboot - no SSH required
- **Automatic recovery**: Server crashes are handled without manual intervention
- **Always up-to-date**: System updates applied on every reboot
- **Production-ready**: Proper service management with logging and monitoring

### Manual Local Build

#### Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake libssl-dev

# Fedora/RHEL
sudo dnf install gcc-c++ cmake openssl-devel
```

#### Build

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
# comments are supported
/ ./public/index.html text/html
/404 ./public/404.html text/html

/css/style.css ./public/css/style.css text/css
/js/app.js ./public/js/app.js application/javascript
```

### SSL Certificates

#### Production (Let's Encrypt)

The deployment pipeline automatically obtains and renews production SSL certificates from Let's Encrypt:

- **Initial Setup**: `install.sh` uses certbot to obtain certificates for `jackthake.com` and `www.jackthake.com`
- **Auto-Renewal**: Systemd timer runs twice daily to check and renew certificates before expiration
- **Zero-Downtime**: Renewal process temporarily stops the server, renews certificates, and restarts automatically
- **Certificate Location**: Certificates are symlinked from `/etc/letsencrypt/live/jackthake.com/` to `./secret/`

Check certificate status:
```bash
# View certificate expiration
sudo certbot certificates

# Check renewal timer status
sudo systemctl status certbot-renew.timer

# View renewal logs
sudo journalctl -u certbot-renew.service

# Manually test renewal (dry-run)
sudo certbot renew --dry-run
```

#### Development (Self-Signed)

For local development, use self-signed certificates in `./secret/`:
```bash
openssl req -x509 -newkey rsa:4096 -keyout secret/server.key -out secret/server.crt -days 365 -nodes -subj "/CN=localhost"
```

- `server.crt` - SSL certificate
- `server.key` - Private key

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
[10/29/25 11:05:22]: THREAD POOl: Creating thread pool of size: 4
[10/29/25 11:05:22]: ROUTER: Attached route / to file path ./public/index.html.
[10/29/25 11:05:22]: ROUTER: Attached route /404 to file path ./public/404.html.
[10/29/25 11:05:22]: ROUTER: Attached route /css/style.css to file path ./public/css/style.css.
[10/29/25 11:05:22]: SERVER: Server intialised using file descriptor 4, on port 443

```

## License

This project is provided as-is for educational and portfolio purposes.
