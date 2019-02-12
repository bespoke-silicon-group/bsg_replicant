#!/bin/bash

# UserData script that gets sent to AWS EC2 instance to start the build process

git clone https://bitbucket.org/taylor-bsg/bsg_f1.git /home/centos/bsg_f1/

cd /home/centos/bsg_f1
git checkout aws_configuration

sudo chown -R centos:centos /home/centos/bsg_f1

sudo su - centos -c "cd bsg_f1/scripts/configuration/; ./setup.sh"
