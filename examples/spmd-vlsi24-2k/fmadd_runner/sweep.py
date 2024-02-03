#!/usr/bin/env python3
import sys
sys.path.append("../../../../../bsg_hammerhead_private/main_board/common/software/")

import subprocess
from hb_board_v0p5_lib import *
from boot import _boot_wrapper as start_asic

start_asic(
    voltage=0.8,
    pwm=50,
    clk_0=(True,0,0),
    clk_1=(True,0,0),
    clk_2=(True,0,0),
    clk_3=(True,0,0),
)

start_asic(
    voltage=0.8,
    pwm=20,
    clk_0=(False,0,0),
    clk_1=(False,0,0),
    clk_2=(False,0,0),
    clk_3=(False,0,0),
)
