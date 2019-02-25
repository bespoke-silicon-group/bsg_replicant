#!/bin/bash

# Script to update kernel packages (just in case)

sudo yum -y update
sudo yum -y clean all
sudo yum -y autoremove
