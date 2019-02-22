#!/bin/sh

AGFI_ID_4x4=agfi-0197253fe8ad909c4
AGFI_ID_12x12=agfi-06947dee5d572d457
AGFI_ID_4x4_v2=agfi-0fa354dc13c33104f
AGFI_ID_4x4_v3=agfi-07b923bc1452f8ecb
sudo fpga-clear-local-image -S 0
sudo fpga-load-local-image -S 0 -I $AGFI_ID_4x4_v3
