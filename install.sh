#!/bin/bash

# Update system
sudo yum update -y

# Install development tools (gcc, g++, make, etc.)
sudo yum groupinstall "Development Tools" -y

# Set Timezone TO PST for logging consistency
sudo timedatectl set-timezone America/Los_Angeles

# Install commonly needed libraries
sudo yum install gcc-c++ make git cmake openssl-devel -y

# Install certbot for Let's Encrypt SSL certificates
sudo yum install -y certbot
cd ~
git clone https://github.com/jackwthake/secure-serve.git
cd secure-serve/

# Make scripts executable
sudo chmod +x on_reboot.sh
sudo chmod +x renew_certs.sh

# Copy service files to systemd directory
# secure-serve-reboot.service runs the on_reboot.sh script at startup and updates the server
# secure-serve.service manages the server process with auto-restart
# certbot-renew.service/timer handles automatic certificate renewals
sudo cp secure-serve-reboot.service /etc/systemd/system/
sudo cp secure-serve.service /etc/systemd/system/
sudo cp certbot-renew.service /etc/systemd/system/
sudo cp certbot-renew.timer /etc/systemd/system/

# Reload systemd to recognize the new services
sudo systemctl daemon-reload

# Enable the reboot service to run at boot (but don't start it now)
sudo systemctl enable secure-serve-reboot.service

# Enable the server service to run at boot
sudo systemctl enable secure-serve.service

# Enable the certificate renewal timer
sudo systemctl enable certbot-renew.timer
sudo systemctl start certbot-renew.timer

# Obtain Let's Encrypt SSL certificates
# Certbot will use port 80 for verification, then we'll use certs with our HTTPS server on port 443
sudo certbot certonly --standalone --non-interactive --agree-tos --email jackthake@hotmail.com \
  -d jackthake.com -d www.jackthake.com --http-01-port 80

# Create secret directory and symlink to Let's Encrypt certificates
mkdir -p secret
sudo ln -sf /etc/letsencrypt/live/jackthake.com/fullchain.pem secret/server.crt
sudo ln -sf /etc/letsencrypt/live/jackthake.com/privkey.pem secret/server.key

# Make certificate directories readable (needed for server to access certs)
sudo chmod 755 /etc/letsencrypt/live
sudo chmod 755 /etc/letsencrypt/archive
sudo chmod 755 /etc/letsencrypt/live/jackthake.com
sudo chmod 755 /etc/letsencrypt/archive/jackthake.com

# Build and start server
mkdir build && cd build
cmake ..
cmake --build .

# Start the server service
sudo systemctl start secure-serve.service
