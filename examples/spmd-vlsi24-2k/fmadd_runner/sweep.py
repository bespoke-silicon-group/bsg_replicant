#!/usr/bin/env python3
import sys
sys.path.append("../../../../../bsg_hammerhead_private/main_board/common/software/")

import time
import subprocess
from hb_board_v0p5_lib import *
from boot import _boot_wrapper as start_asic

fid = open('results.csv', 'w')

print(f"voltage,best_osc", file=fid)
for v_mv in range(550, 900 + 50, 50):
    for osc in range(0, 32):
        v = v_mv / 1000.0
        print(f"[SWEEP] try volt={v}, osc={osc}")

        time.sleep(1.5)
        start_asic(
            voltage=v,
            pwm=70,
            clk_0=(True,osc,0),
            clk_1=(True,osc,0),
            clk_2=(True,osc,0),
            clk_3=(True,osc,0),
        )
        time.sleep(1.5)

        proc = subprocess.run(["timeout 10s make clean exec.log"], shell=True)
        if proc.returncode != 0:
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

    print(f"{v},{osc-1}", file=fid)

fid.close()
