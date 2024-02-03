#!/usr/bin/env python3
import sys
sys.path.append("../../../../../bsg_hammerhead_private/main_board/common/software/")

import time
import subprocess
from hb_board_v0p5_lib import *
from boot import _boot_wrapper as start_asic

for osc in range(32):
    print(f"[SWEEP] try osc={osc}")
    time.sleep(1.5)
    start_asic(
        voltage=0.8,
        pwm=50,
        clk_0=(True,osc,0),
        clk_1=(True,osc,0),
        clk_2=(True,osc,0),
        clk_3=(True,osc,0),
    )
    time.sleep(1.5)
    try:
        subprocess.run(["make clean exec.log"], shell=True, timeout=10)
    except Exception:
        print(traceback.format_exc())
        break

time.sleep(1.5)
start_asic(
    voltage=0.8,
    pwm=20,
    clk_0=(False,0,0),
    clk_1=(False,0,0),
    clk_2=(False,0,0),
    clk_3=(False,0,0),
)
