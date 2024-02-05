#!/usr/bin/env python3
import sys
sys.path.append("../../../../../bsg_hammerhead_private/main_board/common/software/")

import time
import subprocess
from hb_board_v0p5_lib import *
from boot import _boot_wrapper as start_asic

results_csv = "results/results.csv"
confirm_csv = "confirm/confirm.csv"

sleep_time = 1.5
timeout = 15
window = 3

confirm_fid = open(confirm_csv, "w")
print("voltage,osc,clk,samples,avg_current,avg_local_temp_0,avg_remote_temp_0,avg_remote_temp_1", file=confirm_fid)
confirm_fid.close()

results_fid = open(results_csv, "r")

for i,line in enumerate(results_fid):
    if i == 0:
        continue
    v_str, osc_str = line.strip().split(",")
    v = float(v_str)
    v_mv = int(v * 1000)
    osc = int(osc_str)

    while (osc >= 0):
        print(f"[CONFIRM] testing v={v} and osc={osc}")

        time.sleep(sleep_time)
        start_asic(
            voltage=v,
            pwm=100,
            clk_0=(True,osc,0),
            clk_1=(True,osc,0),
            clk_2=(True,osc,0),
            clk_3=(True,osc,0),
        )
        time.sleep(sleep_time)

        print(f"[CONFIRM] launching program")
        proc = subprocess.Popen([f"timeout {timeout}s make clean exec.log"], shell=True, stderr=subprocess.STDOUT, stdout=subprocess.PIPE)
        time.sleep(3.0)
        print(f"[CONFIRM] starting monitor")

        monitor_current = []
        monitor_local_temp_0 = []
        monitor_remote_temp_0 = []
        monitor_remote_temp_1 = []

        start_time = time.time()
        while time.time() - start_time < window:
            monitor_current.append(tpsm831d31_read_total_current())
            monitor_local_temp_0.append(tmp422_read_temperature(0))
            monitor_remote_temp_0.append(tmp422_read_temperature(1))
            monitor_remote_temp_1.append(tmp422_read_temperature(2))

        print(f"[CONFIRM] stopping monitor")

        stdout, _ = proc.communicate()

        output = stdout.decode("utf-8")
        with open(f"confirm/confirm_v{v_mv}_osc{osc}.log", "w") as log_fid:
            print(output, file=log_fid)

        if proc.returncode != 0:
            print(f"[CONFIRM] program timed out")
            osc -= 1
            continue

        elif output.find("FAILED") != -1:
            print(f"[CONFIRM] program returned incorrect data")
            osc -= 1
            continue

        else:
            print(f"[CONFIRM] program finished succesfully")
            break

    time.sleep(sleep_time)
    start_asic(
        voltage=0.8,
        pwm=50,
        clk_0=(False,0,0),
        clk_1=(False,0,0),
        clk_2=(False,0,0),
        clk_3=(False,0,0),
    )

    print(f"[CONFIRM] program finished on v={v} and osc={osc}")

    clk = input("What was the clock speed? ")
    samples = len(monitor_current)
    avg_current = sum(monitor_current) / samples
    avg_local_temp_0 = sum(monitor_local_temp_0) / samples
    avg_remote_temp_0 = sum(monitor_remote_temp_0) / samples
    avg_remote_temp_1 = sum(monitor_remote_temp_1) / samples

    confirm_fid = open(confirm_csv, "a")
    print(f"%d,%d,%s,%d,%0.3f,%0.3f,%0.3f,%0.3f" % (v_mv, osc, clk, samples, avg_current, avg_local_temp_0, avg_remote_temp_0, avg_remote_temp_1), file=confirm_fid)
    confirm_fid.close()

results_fid.close()
