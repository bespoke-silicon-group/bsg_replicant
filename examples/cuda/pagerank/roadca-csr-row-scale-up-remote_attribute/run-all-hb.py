import sys
import re
import os
import json
import copy
import subprocess
import pathlib

sbatch_template = "cd {path}\n"
sbatch_template += "module load synopsys-2020/synopsys-vcs-R-2020.12\n"
sbatch_template += "export BRG_BSG_BLADERUNNER_DIR=/work/global/zz546/bsg_bladerunner\n"
sbatch_template += "export BSG_MACHINE=pod_X1Y1_ruche_X16Y8_hbm_one_pseudo_channel\n"
sbatch_template += "export BSG_MACHINE_PATH=$BRG_BSG_BLADERUNNER_DIR/bsg_replicant/machines/$BSG_MACHINE\n"
sbatch_template += "make pc-histogram.log > out.log 2>&1\n"

def fancy_print(route):
  for waypoint in route:
    print(waypoint)

#with open('1iter_graphsage_int.json',) as f:
#  route = json.load(f)

#fancy_print(route)
#print()

#print("total number of jobs: " + str(len(route)))

parallel_num = 16
seq_num = int(64 / parallel_num)

mod = 64 % parallel_num

assert mod == 0, "parallel_num should be divisible by 64 !!"

for s in range(seq_num):
    cosim_run = {}
    for i in range(parallel_num):
        # create kernel folder
        pod_num = s * parallel_num + i
        name = "block_%d" % pod_num
        sh_cmd = "mkdir -p " + name
        print(sh_cmd)
        os.system(sh_cmd)

        sh_cmd = "(cd " + name + "; cp ../Makefile .; cp ../pr_csr_row_block.cpp .)"
        print(sh_cmd)
        os.system(sh_cmd)

        current_path = str(pathlib.Path(__file__).parent.absolute())
        openfile = current_path + "/" + name + "/Makefile"
        print(openfile)

        # Modify the macros in Makefile
        f = open(openfile, 'r')
        alllines = f.readlines()
        f.close()
        f = open(openfile, 'w+')
        host_pod = "CXXDEFINES += -DSIM_CURRENT_POD=" + str(i)
        kernel_pod = "RISCV_DEFINES += -DSIM_KERNEL_CURRENT_POD=" + str(i)
        for eachline in alllines:
            if eachline.__contains__("CXXDEFINES += -DSIM_CURRENT_POD=1"):
                newline = eachline.replace("CXXDEFINES += -DSIM_CURRENT_POD=1", host_pod)
                f.write(newline)
            elif eachline.__contains__("RISCV_DEFINES += -DSIM_KERNEL_CURRENT_POD=1"):
                newline = eachline.replace("RISCV_DEFINES += -DSIM_KERNEL_CURRENT_POD=1", kernel_pod)
                f.write(newline)
            else:
                f.write(eachline)
        f.close()

        # get current path 
        path = str(os.path.abspath(os.getcwd())) + "/" + name
        print(path)
        
        # generate qsub script
        sbatch_starter = sbatch_template.format(job_name=name, path=path)
        print(sbatch_starter)
        with open(name + "/run.sh", 'w') as outfile:
            outfile.write(sbatch_starter)

        print("starting cosim job ...")
        thread = i
        cosim_run[thread] = subprocess.Popen(["sh", name + "/run.sh"], env=os.environ)
    for k in range(parallel_num):
        cosim_run[k].wait()
