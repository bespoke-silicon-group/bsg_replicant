#!/bin/sh

AGFI_ID=agfi-0523a31c4d2804b51

sudo fpga-clear-local-image -S 0
sudo fpga-load-local-image -S 0 -I $AGFI_ID
