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
cyclic_each_pod = np.zeros((pod_num), dtype=int)
block_each_pod = np.zeros((pod_num), dtype=int)


for i in range(pod_num):
    block_name = "block_%d" % i
    # collect full stack
    current_path = str(pathlib.Path(__file__).parent.absolute())
    openfile = current_path + "/" + block_name + "/stats/manycore_stats.log"
    block_file = "/work/global/zz546/bigblade-6.4/bsg_replicant/examples/cuda/pagerank/pokec-csr-row-scale-up/" + block_name + "/stats/manycore_stats.log"

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
            cyclic_each_pod[i] = int(block_data[5])

    f1 = open(block_file, 'r')
    alllines1 = f1.readlines()
    f1.close()
    for eachline in alllines1:
        if eachline.__contains__("Runtime of sub-block"):
            print(eachline)
            block_data = eachline.split()
            block_each_pod[i] = int(block_data[5])

print(cyclic_each_pod)
cyclic_max = np.amax(cyclic_each_pod, axis = 0)
print("Max of cyclic partition is: ", cyclic_max)
print(block_each_pod)
block_max = np.amax(block_each_pod, axis = 0)
print("Max of block partition is: ", block_max)
block_each_pod = np.sort(block_each_pod)
cyclic_each_pod = np.sort(cyclic_each_pod)
plt.plot(np.arange(0, 64), block_each_pod)
plt.plot(np.arange(0, 64), cyclic_each_pod)
plt.show()
plt.savefig('pokec-block-cyclic.png')


            
