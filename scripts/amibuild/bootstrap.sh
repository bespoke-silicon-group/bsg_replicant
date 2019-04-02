#!/bin/bash

# UserData script that gets sent to AWS EC2 instance to start the build process

cd /home/centos/
git clone https://github.com/bespoke-silicon-group/$release_repo.git 
cd $release_repo
git checkout $release_hash

sudo chown -R centos:centos /home/centos/$release_repo

sudo su - centos -c "make -C /home/centos/$release_repo -f Makefile.amibuild -j4 install" > amibuild.log
