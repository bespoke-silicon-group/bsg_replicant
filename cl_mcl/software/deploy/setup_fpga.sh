#!/bin/sh

#AGFI_ID=agfi-004414092006d3159
#AGFI_ID=agfi-0119fd2a4eea074bf
AGFI_ID=agfi-0caaee3f6b5c983eb

sudo fpga-clear-local-image -S 0
sudo fpga-load-local-image -S 0 -I $AGFI_ID
