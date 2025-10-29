#!/bin/bash

# Update system
sudo yum update -y

# Install development tools (gcc, g++, make, etc.)
sudo yum groupinstall "Development Tools" -y

# Set Timezone TO PST for logging consistency
sudo timedatectl set-timezone America/Los_Angeles

# Install commonly needed libraries
sudo yum install gcc-c++ make git cmake openssl-devel -y
cd ~
git clone https://github.com/jackwthake/secure-serve.git
cd secure-serve/

# Make the on_reboot.sh script executable
sudo chmod +x on_reboot.sh

# Copy service files to systemd directory
# secure-serve-reboot.service runs the on_reboot.sh script at startup and updates the server
# secure-serve.service manages the server process with auto-restart
sudo cp secure-serve-reboot.service /etc/systemd/system/
sudo cp secure-serve.service /etc/systemd/system/

# Reload systemd to recognize the new services
sudo systemctl daemon-reload

# Enable the reboot service to run at boot (but don't start it now)
sudo systemctl enable secure-serve-reboot.service

# Enable the server service to run at boot
sudo systemctl enable secure-serve.service

# Generate self signed certificates
mkdir secret
openssl req -x509 -newkey rsa:4096 -keyout secret/server.key -out secret/server.crt -days 365 -nodes \
  -subj "/CN=localhost"

# Build and start server
mkdir build && cd build
cmake ..
cmake --build .

# Start the server service
sudo systemctl start secure-serve.service
