# AWS Boot/Environment Setup Scripts

## Overview
We want the vivado/aws environment to be setup when instances launch or reboot.
As it turns out, `cron` will trigger `@reboot` lines at both these events.

The following needs to be done for the AMI:
1. Move `setup_environment.sh` into `/usr/local/bin`
2. Add the following line to the crontab (`crontab -e`):
    `@reboot /usr/local/bin/setup_environment.sh`
3. Make sure the `cron` service is started:
    `sudo chkconfig crond on`  (on centos)
    `sudo systemctl enable crond.service` (on other distros)

## File list
`setup_environment.sh` -- Script for `cron` to run that sets up the environment

## Potential Issues
The AMI used for testing was the FPGA Developer AMI-1.5.0. Using `systemctl`
works fine for this instance. Apparently, `@reboot` should also run after a
crash or a hard shutdown, but `cron` implementations vary, however it's
extremely unlikely that these issues will be encountered on AWS.
