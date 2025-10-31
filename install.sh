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

# Clone repo as ec2-user to their home directory
sudo -u ec2-user git clone https://github.com/jackwthake/secure-serve.git /home/ec2-user/secure-serve
sudo -u ec2-user git config --global --add safe.directory /home/ec2-user/secure-serve
cd /home/ec2-user/secure-serve

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

# Read domain from config file
DOMAIN=$(grep "^domain=" secure-serve.conf | cut -d'=' -f2 | tr -d ' ')
if [ -z "$DOMAIN" ]; then
    echo "ERROR: domain not found in secure-serve.conf"
    exit 1
fi

# Read email from environment variable (must be set in user_data.sh)
EMAIL="$SECURE_SERVE_EMAIL"
if [ -z "$EMAIL" ]; then
    echo "ERROR: SECURE_SERVE_EMAIL environment variable must be set"
    exit 1
fi

echo "Using domain: $DOMAIN"
echo "Using email: $EMAIL"

# Obtain Let's Encrypt SSL certificates
# Certbot will use port 80 for verification, then we'll use certs with our HTTPS server on port 443
sudo certbot certonly --standalone --non-interactive --agree-tos --email "$EMAIL" \
  -d "$DOMAIN" -d "www.$DOMAIN" --http-01-port 80

# Create secret directory and symlink to Let's Encrypt certificates (as ec2-user)
sudo -u ec2-user mkdir -p secret
sudo ln -sf "/etc/letsencrypt/live/$DOMAIN/fullchain.pem" secret/server.crt
sudo ln -sf "/etc/letsencrypt/live/$DOMAIN/privkey.pem" secret/server.key

# Make certificate directories readable (needed for server to access certs)
sudo chmod 755 /etc/letsencrypt/live
sudo chmod 755 /etc/letsencrypt/archive
sudo chmod 755 "/etc/letsencrypt/live/$DOMAIN"
sudo chmod 755 "/etc/letsencrypt/archive/$DOMAIN"

# Build and start server (as ec2-user)
sudo -u ec2-user mkdir -p logs build
cd build
sudo -u ec2-user cmake ..
sudo -u ec2-user cmake --build .

# Start the server service
sudo systemctl start secure-serve.service
