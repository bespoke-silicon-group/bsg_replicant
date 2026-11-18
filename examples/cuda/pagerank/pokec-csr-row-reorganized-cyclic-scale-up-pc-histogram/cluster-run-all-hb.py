import sys
import re
import os
import json
import copy
import subprocess
import pathlib

sbatch_template = "#!/usr/bin/sh\n"
sbatch_template += "#SBATCH -J {job_name}\n"
sbatch_template += "#SBATCH -o {path}/{job_name}.out\n"
sbatch_template += "#SBATCH -e {path}/{job_name}.err\n"
sbatch_template += "#SBATCH -N 1\n"
sbatch_template += "#SBATCH -n 1\n"
sbatch_template += "#SBATCH --get-user-env\n"
sbatch_template += "#SBATCH --mem=16GB\n"
sbatch_template += "#SBATCH -t 72:00:00\n"
sbatch_template += "#SBATCH --partition=cpu\n"
sbatch_template += "cd {path}\n"
sbatch_template += "module load synopsys-2020/synopsys-vcs-R-2020.12\n"
sbatch_template += "export BRG_BSG_BLADERUNNER_DIR=/work/global/zz546/bsg_bladerunner\n"
sbatch_template += "export BSG_MACHINE=pod_X1Y1_ruche_X16Y8_hbm_one_pseudo_channel\n"
sbatch_template += "export BSG_MACHINE_PATH=$BRG_BSG_BLADERUNNER_DIR/bsg_replicant/machines/$BSG_MACHINE\n"
sbatch_template += "make pc-histogram.log > out.log 2>&1\n"
sbatch_template += "make kernel.dis > kernel.txt 2>&1\n"

def fancy_print(route):
  for waypoint in route:
    print(waypoint)

#with open('1iter_graphsage_int.json',) as f:
#  route = json.load(f)

#fancy_print(route)
#print()

#print("total number of jobs: " + str(len(route)))


for i in range(64):
    name = "block_%d" % i
    sh_cmd = "mkdir -p " + name
    print(sh_cmd)
    os.system(sh_cmd)

    sh_cmd = "(cd " + name + "; cp ../Makefile .; cp ../pr_csr_reorganized_cyclic.cpp .)"
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
    with open(name + "/sbatch.sh", 'w') as outfile:
        outfile.write(sbatch_starter)

    print("starting cosim job ...")
    cosim_run = subprocess.Popen(["sbatch", name + "/sbatch.sh"], env=os.environ)
