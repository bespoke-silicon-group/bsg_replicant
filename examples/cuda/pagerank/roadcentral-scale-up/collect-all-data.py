import sys
import os
import json
import copy
import subprocess
import numpy as np
import pathlib
np.set_printoptions(threshold=sys.maxsize)

pod_num = 64
abs_each_block = np.zeros((pod_num, pod_num), dtype=int)

for i in range(pod_num):
  for j in range(5):
    block_name = "block_%d_%d" % (i, j)

    current_path = str(pathlib.Path(__file__).parent.absolute())
    openfile = current_path + "/" + block_name + "/stats/manycore_stats.log"
    if not os.path.exists(openfile):
        sh_cmd = "(cd " + block_name + "; python /work/global/zz546/bigblade-6.4/bsg_manycore/software/py/vanilla_parser/stats_parser.py --stats vanilla_stats.csv --vcache-stats vcache_stats.csv --tile)"
        print(sh_cmd)
        os.system(sh_cmd)
    
    f = open(openfile, 'r')
    alllines = f.readlines()
    f.close()
    for eachline in alllines:
        if eachline.__contains__("Runtime of sub-block"):
            block_data = eachline.split()
            block_num = int(block_data[3]) + j * 13 - 2
            abs_each_block[block_num, i] = int(block_data[5])

max_array = np.amax(abs_each_block, axis = 1)

scale_up_sum = np.sum(max_array)
print(abs_each_block)
print(max_array)
print("Total time of the scale up model is: ", scale_up_sum)



            
