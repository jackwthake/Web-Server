#!/bin/bash

LOGFILE="$HOME/secure-serve/logs/reboot.log"

echo "=== Reboot script started at $(date) ===" >> "$LOGFILE"

# Update system (needs sudo)
echo "Updating system..." >> "$LOGFILE"
sudo yum update -y >> "$LOGFILE" 2>&1

cd "$HOME/secure-serve" || exit 1

# download any updates from git (no sudo needed as ec2-user)
echo "Pulling latest code from GitHub..." >> "$LOGFILE"
git pull origin main >> "$LOGFILE" 2>&1

# rebuild and run server
cd build || exit 1
echo "Building server..." >> "$LOGFILE"
cmake -B . -S .. >> "$LOGFILE" 2>&1
cmake --build . >> "$LOGFILE" 2>&1

# Restart the server service (needs sudo)
echo "Restarting server service..." >> "$LOGFILE"
sudo systemctl restart secure-serve.service >> "$LOGFILE" 2>&1

echo "=== Reboot script completed at $(date) ===" >> "$LOGFILE"
echo "" >> "$LOGFILE"