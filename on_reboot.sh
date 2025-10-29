#!/bin/bash

# Update system
sudo yum update -y

cd /home/ec2-user/secure-serve || exit 1

# download any updates from git
git pull origin main

# rebuild and run server
cd build || exit 1
cmake -B . -S ..
cmake --build .

# Restart the server service
sudo systemctl restart secure-serve.service