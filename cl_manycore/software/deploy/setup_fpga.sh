#!/bin/sh

#AGFI_ID=agfi-004414092006d3159
AGFI_ID=agfi-0119fd2a4eea074bf


sudo fpga-clear-local-image -S 0
sudo fpga-load-local-image -S 0 -I $AGFI_ID
