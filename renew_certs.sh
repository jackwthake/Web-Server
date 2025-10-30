#!/bin/bash

# Renew Let's Encrypt certificates
# This script runs periodically to renew certificates before they expire

# Stop the server to free up port 80 for certbot verification
sudo systemctl stop secure-serve.service

# Attempt certificate renewal
sudo certbot renew --standalone --http-01-port 80

# Restart the server
sudo systemctl start secure-serve.service

# Log the renewal attempt
echo "[$(date '+%m/%d/%y %H:%M:%S')]: Certificate renewal attempted" >> ~/secure-serve/logs/cert_renewal.log
echo "[$(date '+%m/%d/%y %H:%M:%S')]: CERT: Certificate renewal attempted" >> ~/secure-serve/logs/server.log
