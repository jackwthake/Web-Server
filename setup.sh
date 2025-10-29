#!/bin/bash

# Update system
sudo yum update -y

# Install development tools (gcc, g++, make, etc.)
sudo yum groupinstall "Development Tools" -y

# Set Timezone TO PST for logging consistency
sudo timedatectl set-timezone America/Los_Angeles

# Install commonly needed libraries
sudo yum install gcc-c++ make git cmake openssl-devel -y
git clone https://github.com/jackwthake/secure-serve.git
cd secure-serve/

# Generate self signed certificates
mkdir secret
openssl req -x509 -newkey rsa:4096 -keyout secret/server.key -out secret/server.crt -days 365 -nodes \
  -subj "/CN=localhost"

# Build and run server
mkdir build && cd build
cmake ..
cmake --build .
sudo nohup ./serve &
