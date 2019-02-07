#!/bin/bash

# This script copies the environment setup script from the bsg_f1 repository
# into /env/profile.d/ where it is run when a user logs in

echo "export AGFI=$1" | sudo tee /etc/profile.d/agfi.sh
sudo cp /home/centos/bsg_f1/scripts/amiconfig/profile.d_bsg.sh /etc/profile.d/
