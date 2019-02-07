#!/bin/bash

# UserData script that gets sent to AWS EC2 instance to start the build process

cd /home/centos/
git clone https://bitbucket.org/taylor-bsg/bsg_f1.git 
cd bsg_f1 
git checkout $bsg_f1_commit_id

sudo chown -R centos:centos /home/centos/bsg_f1

sudo su - centos -c "cd ~/bsg_f1/scripts/amiconfig/; ./setup.sh $aws_ver $agfi $dependencies"
