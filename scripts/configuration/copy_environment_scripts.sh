#!/bin/bash

# This script copies the environment setup script from the bsg_f1 repository
# into /env/profile.d/ where it is run when a user logs in

sudo cp /home/centos/bsg_f1/scripts/boot/profile.d_bsg.sh /etc/profile.d/
