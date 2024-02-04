#!/usr/bin/env python3
import sys
sys.path.append("../../../../../bsg_hammerhead_private/main_board/common/software/")

import time
import subprocess
from hb_board_v0p5_lib import *
from boot import _boot_wrapper as start_asic

results_csv = "results.csv"
v_min = 560
v_max = 560
v_step = 20

print(f"[SWEEP] starting sweep from v_min={v_min} to v_max={v_max} with v_step={v_step}")

fid = open(results_csv, 'w')
print(f"voltage,best_osc", file=fid)
fid.close()

for v_mv in range(v_min, v_max + v_step, v_step):
    v = v_mv / 1000.0
    best_osc = -1
    osc_settings = list(range(32))

    while len(osc_settings) != 0:
        osc_idx = len(osc_settings)//2
        osc = osc_settings[osc_idx]

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

        print(f"[SWEEP] launching program")
        proc = subprocess.run(["timeout 10s make clean exec.log"], shell=True, stderr=subprocess.STDOUT, stdout=subprocess.PIPE)

        output = proc.stdout.decode("utf-8")
        with open(f"sweep_exec_v{v_mv}_osc{osc}", "w") as log_fid:
            print(output, file=log_fid)

        if proc.returncode != 0:
            print(f"[SWEEP] program timed out")
            osc_settings = osc_settings[:osc_idx]

        elif output.find("FAILED") != -1:
            print(f"[SWEEP] program returned incorrect data")
            osc_settings = osc_settings[:osc_idx]

        else:
            print(f"[SWEEP] program finished succesfully")
            best_osc = osc
            osc_settings = osc_settings[osc_idx+1:]

    time.sleep(1.5)
    start_asic(
        voltage=0.8,
        pwm=20,
        clk_0=(False,0,0),
        clk_1=(False,0,0),
        clk_2=(False,0,0),
        clk_3=(False,0,0),
    )

    print(f"[SWEEP] finished sweeping for v={v}, best osc={best_osc}")

    fid = open(results_csv, 'w')
    print(f"{v},{best_osc}", file=fid)
    fid.close()
