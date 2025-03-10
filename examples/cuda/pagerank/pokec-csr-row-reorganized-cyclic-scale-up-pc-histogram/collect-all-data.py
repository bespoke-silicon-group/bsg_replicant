import sys
import os
import json
import copy
import subprocess
import numpy as np
import pathlib
import matplotlib.pyplot as plt

# ======================================================
# Main loop
# ======================================================

pod_num = 64
abs_each_block = np.zeros((pod_num), dtype=int)
nonzero_pods = np.zeros((pod_num, 2), dtype=int)
percent = np.zeros((pod_num), dtype=float)
sort_norm = np.zeros((pod_num), dtype=float)

for i in range(pod_num):
    block_name = "block_%d" % i
    # collect full stack
    current_path = str(pathlib.Path(__file__).parent.absolute())
    openfile = current_path + "/" + block_name + "/stats/manycore_stats.log"
    logfile = current_path + "/" + block_name + "/out.log"

    if not os.path.exists(openfile):
        sh_cmd = "(cd " + block_name + "; python /work/global/zz546/bigblade-6.4/bsg_manycore/software/py/vanilla_parser/stats_parser.py --stats vanilla_stats.csv --vcache-stats vcache_stats.csv --tile --cache-line-words 16)"
        print(sh_cmd)
        os.system(sh_cmd)
    
    f = open(openfile, 'r')
    alllines = f.readlines()
    f.close()
    for eachline in alllines:
        if eachline.__contains__("Runtime of sub-block"):
            print(eachline)
            block_data = eachline.split()
            abs_each_block[i] = int(block_data[5])

    f1 = open(logfile, 'r')
    alllines1 = f1.readlines()
    f1.close()
    for eachline in alllines1:
        if eachline.__contains__("Non-zero rows"):
            print(eachline)
            nonzero_data = eachline.split()
            nonzero_pods[i, 0] = int(nonzero_data[2])
            nonzero_pods[i, 1] = int(nonzero_data[5])
            percent[i] = nonzero_pods[i, 0] / nonzero_pods[i, 1]

scale_up = np.amax(abs_each_block, axis = 0)
scaleup_min = np.amin(abs_each_block, axis = 0)
gap = scaleup_min / scale_up
print(abs_each_block)
print(nonzero_pods)
print("Percent of each pod", percent)
print("Total time of the scale up model is: ", scale_up)
print("Gap is ", gap)
#nonzero_sum = np.sum(nonzero_pods, axis = 0)
#print("Total non-zero rows and rows ", nonzero_sum)
#total_percent = float(nonzero_sum[0]) / float(nonzero_sum[1])
#print("Non-zero percent of the whole graph", total_percent)
sort_norm = abs_each_block / scale_up
sort_norm = np.sort(sort_norm)
print("Sorted normalized runtime breakdown: ", sort_norm)
plt.plot(np.arange(0, 64), sort_norm)
plt.ylim([0, 1])
plt.show()
plt.savefig('pokec-csr-reorganized-cyclic-distribution.png')


            
