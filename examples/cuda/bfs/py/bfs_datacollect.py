import glob
import subprocess
import re
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--logpath", type=str, default="./stat.log", 
                        help="path and name of the result log")
parser.add_argument("--graphpattern", type=str, default="./pokec*", 
                        help="path and name pattern of the working directories")
args = parser.parse_args()


aggr_inst = 0
aggr_cyc = 0
abs_cyc = 0
aggr_stall = 0
dram_stall = 0
cache_miss = 0
cache_total = 0
traversed_edges = 0

abs_seq = {}
cache_idle = {}
cache_bandwidth = {}
cache_missrate = {}
file_name_wr = args.logpath
f_ptr      = open(file_name_wr, "w")

for name in glob.glob(args.graphpattern):
    #openfile=subprocess.run(["cd",name],check=True)
    #print("open %s result: %d",name,openfile)
    #vanilla_stats = name + "vanilla_stats.csv"
    #vcache_stats = name + "vcache_stats.csv"
    #subprocess.run(["python","/work/shared/users/phd/zy383/HB_Cosim/Bladerunner_6.2.0/bsg_bladerunner/bsg_manycore/software/py/vanilla_parser/stats_parser.py", "--stats", vanilla_stats, "--vcache-stats", vcache_stats],check=True)
    subprocess.run(["python","/work/global/zy383/Bladerunner6.4.0/bsg_manycore/software/py/vanilla_parser/stats_parser.py", "--stats", "vanilla_stats.csv", "--vcache-stats", "vcache_stats.csv"],check=True,cwd=name)
    file_stat = name + "stats/manycore_stats.log"
    file_edge = name + "bfs_stats.txt"
    line_cnt = 0
    line_edgecnt = 0
    
    m = re.search(r'(\w+)_graph-type__(\d+)_vertices__(\d+)_edges__(\d+)_root__'
                      + r'(\d+)_iter__(\d+)_tile-groups__(\d+)_tgx__(\d+)_tgy',
                      name)
    ite = int(m.group(5))
    print(ite)
    with open(file_edge) as fe:
        line = fe.readline()   
        while line:
            if line_edgecnt == 4:
                line_decomp = line.split()
                traversed_edges += int(line_decomp[1]) 
            line_edgecnt += 1    
            line = fe.readline()   


    idel_percentage = 0
    with open(file_stat) as fp:
        line = fp.readline()
        while line:
            if line_cnt == 3:
                #print(line)
                line_decomp = line.split()
                #print(line_decomp[6],line_decomp[7])
                aggr_inst += int(line_decomp[1])
                aggr_cyc += int(line_decomp[5])
                aggr_stall += int(line_decomp[3])
                abs_cyc += int(line_decomp[6])
                abs_seq[ite] = int(line_decomp[6])
            elif line_cnt == 160:
                line_decomp = line.split()
                dram_stall += int(line_decomp[1])
            elif line_cnt == 504:
                line_decomp = line.split()
                cache_miss += int(line_decomp[1])
                cache_total += int(line_decomp[2])
                cache_missrate[ite] = float(line_decomp[3])
                cache_bandwidth[ite] = 64*int(line_decomp[1])/abs_seq[ite]
            elif line_cnt > 524 and line_cnt < 557:
                line_decomp = line.split()
                idel_percentage += float(line_decomp[8])
            line_cnt += 1
            line = fp.readline()
    cache_idle[ite] = idel_percentage/32
     
f_ptr.write("==================abs cycles per iter====================\n")
for count, value in sorted(abs_seq.items(), key=lambda item: item[1],reverse=True):
    f_ptr.write(str(count)+": "+str(value)+"\n")
f_ptr.write("==================cache idle percentage per iter====================\n") 
for count, value in sorted(cache_idle.items(), key=lambda item: item[1]): 
    f_ptr.write(str(count)+": "+str(value)+"\n")   
f_ptr.write("==================cache missrate per iter====================\n") 
for count, value in sorted(cache_missrate.items(), key=lambda item: item[1]): 
    f_ptr.write(str(count)+": "+str(value)+"\n")   
f_ptr.write("==================cache bandwidth per iter====================\n") 
for count, value in sorted(cache_bandwidth.items(), key=lambda item: item[1],reverse=True): 
    f_ptr.write(str(count)+": "+str(value)+"\n")
f_ptr.write("==================total status====================\n") 
f_ptr.write("total instruction: "+str(aggr_inst)+"\n")
f_ptr.write("total abs cycle: "+str(abs_cyc)+"\n")
f_ptr.write("total stall cycle: "+str(aggr_stall)+"\n")
f_ptr.write("total IPC: "+str(aggr_inst/aggr_cyc)+"\n")
f_ptr.write("stall percentage: "+str(aggr_stall/aggr_cyc)+"\n")
f_ptr.write("dram load stall percentage: "+str(dram_stall/aggr_stall)+"\n")
f_ptr.write("total traversed edges: "+str(traversed_edges)+"\n")
exec_time=abs_cyc/1500000
f_ptr.write("total execution time: "+str(exec_time)+" ms\n")
f_ptr.write("throughput in MEPS: "+str(traversed_edges/1000/exec_time)+" MEPS\n")

f_ptr.close()

print("total instruction:",aggr_inst)
print("total abs cycle:",abs_cyc)
print("IPC:",aggr_inst/aggr_cyc)
print("total stall cycle:", aggr_stall)
print("stall percentage:",aggr_stall/aggr_cyc)





    
