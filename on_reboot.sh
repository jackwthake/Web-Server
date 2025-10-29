#!/bin/bash

# Update system
sudo yum update -y

cd ~/secure-serve || exit 1

# download any updates from git
git pull origin main

# rebuild and run server
cd build || exit 1
cmake -B . -S ..
cmake --build .
sudo ./serve &