import glob
import subprocess
import re
import argparse
from os.path import exists as file_exists 

parser = argparse.ArgumentParser()
parser.add_argument("--logpath", type=str, default="./stat.log", 
                        help="path and name of the result log")
parser.add_argument("--dirpattern", type=str, default="pokec", 
                        help="pattern of the working directories")
parser.add_argument("--dirpath", type=str, default="./", 
                        help="path of the working directories")  
parser.add_argument("--ite_start", type=int, default=0, 
                        help="start iteration ID")
parser.add_argument("--ite_num", type=int, default=16, 
                        help="number of iterations")                                               
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
abs_avg = {}
abs_small = {}
largest_out = {}
out_per_ite = {}
cache_idle = {}
cache_bandwidth = {}
cache_missrate = {}
file_name_wr = args.logpath
f_ptr      = open(file_name_wr, "w")

idx_increase = args.ite_start
num_ite = args.ite_num
for i in range(num_ite+1):
    abs_seq[i+idx_increase]=0
    largest_out[i+idx_increase]=0
    out_per_ite[i+idx_increase]=0
    abs_avg[i+idx_increase]=0
    abs_small[i+idx_increase]=10000000000

dirmatch = args.dirpath + args.dirpattern + '*'
for name in glob.glob(dirmatch):
    #openfile=subprocess.run(["cd",name],check=True)
    #print("open %s result: %d",name,openfile)
    #vanilla_stats = name + "vanilla_stats.csv"
    #vcache_stats = name + "vcache_stats.csv"
    #subprocess.run(["python","/work/shared/users/phd/zy383/HB_Cosim/Bladerunner_6.2.0/bsg_bladerunner/bsg_manycore/software/py/vanilla_parser/stats_parser.py", "--stats", vanilla_stats, "--vcache-stats", vcache_stats],check=True)
    #subprocess.run(["python","/work/shared/users/phd/zy383/HB_Cosim/Bladerunner_6.2.0/bsg_bladerunner/bsg_manycore/software/py/vanilla_parser/stats_parser.py", "--stats", "vanilla_stats.csv", "--vcache-stats", "vcache_stats.csv"],check=True,cwd=name)
    logstats = name + "stats/manycore_stats.log"
    log_exists = file_exists(logstats)
    if(not log_exists):
        print("log not found in path ",name)
        break
    file_stat = name + "stats/manycore_stats.log"
    file_edge = name + "bfs_stats.txt"
    file_outfrontier = name + "out_put_lenth.txt"
    line_cnt = 0
    line_edgecnt = 0
    
    m = re.search(r'(\w+)_graph-type__(\d+)_vertices__(\d+)_edges__(\d+)_root__'
                      + r'(\d+)_iter__(\d+)_tile-groups__(\d+)_tgx__(\d+)_tgy__(\d+)_pod',
                      name)
    vertices = int(m.group(2))
    ite = int(m.group(5))
    pod_id = int(m.group(9))
    print(ite,pod_id)

    #if pod_id == 0:
    #    with open(file_edge) as fe:
    #        line = fe.readline()   
    #        while line:
    #            if line_edgecnt == 4:
    #                line_decomp = line.split()
    #                traversed_edges += int(line_decomp[1]) 
    #            line_edgecnt += 1
    #            line = fe.readline() 

    with open(file_outfrontier) as fe:
        line = fe.readline()   
        out_per_ite[ite] = out_per_ite[ite] + int(line)
        if int(line) > largest_out[ite]:
            largest_out[ite] = int(line)

    abs_ite_pod = 0
    idel_percentage = 0
    with open(file_stat) as fp:
        line = fp.readline()
        while line:
            if line_cnt == 3:
                line_decomp = line.split() 
                aggr_inst += int(line_decomp[1])
                aggr_cyc += int(line_decomp[5])
                aggr_stall += int(line_decomp[3])
                abs_ite_pod = int(line_decomp[6])
            if line_cnt == 4:
                #print(line)
                line_decomp = line.split()
                #print(line_decomp[6],line_decomp[7])
                aggr_inst += int(line_decomp[1])
                aggr_cyc += int(line_decomp[5])
                aggr_stall += int(line_decomp[3])
                abs_ite_pod += int(line_decomp[6])
                #print(line_decomp)
                if abs_ite_pod > abs_seq[ite]:
                    #abs_cyc += int(line_decomp[6])
                    abs_seq[ite] = abs_ite_pod
                    print("abs:",abs_ite_pod)
                abs_avg[ite] += abs_ite_pod/64
                if abs_small[ite] > abs_ite_pod:
                    abs_small[ite] = abs_ite_pod
                
            #elif line_cnt == 47 or line_cnt == 72:
            #    line_decomp = line.split()
            #    dram_stall += int(line_decomp[1])
            #elif line_cnt == 640 or line_cnt == 697:
            #    line_decomp = line.split()
            #    cache_miss += int(line_decomp[1])
            #    cache_total += int(line_decomp[2])
            #    #cache_missrate[ite] = float(line_decomp[3])
            #    if line_cnt == 697:
            #        cache_bandwidth[ite] = 64*int(line_decomp[1])/abs_seq[ite]
            #elif line_cnt > 524 and line_cnt < 557:
            #    line_decomp = line.split()
            #    idel_percentage += float(line_decomp[8])
            line_cnt += 1
            line = fp.readline()
    #cache_idle[ite] = idel_percentage/32
for ite,time in abs_seq.items():
    abs_cyc += time
    print("absolute cycle in ite",ite,"is ",time)

out_total = 0
for ite,outsize in largest_out.items():
    print("output frontier size in ite",ite,"is ",outsize)
    if out_per_ite[ite] < vertices*0.1:
        if out_per_ite[ite] < 128000:
            out_total += outsize*63
        else:
            out_total += outsize*63*8
    else:
        if (vertices/8) < 512000:
            out_total += vertices/8
        else:
            out_total += vertices
print("total out size,",out_total)


     
f_ptr.write("==================abs cycles per iter====================\n")
for count, value in sorted(abs_seq.items(), key=lambda item: item[1],reverse=True):
    f_ptr.write(str(count)+": "+str(value)+"\n")
f_ptr.write("==================average abs cycles per iter====================\n")
for count, value in sorted(abs_avg.items(), key=lambda item: item[1],reverse=True):
    f_ptr.write(str(count)+": "+str(value)+"\n")
f_ptr.write("==================smallest abs cycles per iter====================\n")
for count, value in sorted(abs_small.items(), key=lambda item: item[1],reverse=True):
    f_ptr.write(str(count)+": "+str(value)+"\n")
#f_ptr.write("==================cache idle percentage per iter====================\n") 
#for count, value in sorted(cache_idle.items(), key=lambda item: item[1]): 
#    f_ptr.write(str(count)+": "+str(value)+"\n")   
#f_ptr.write("==================cache missrate per iter====================\n") 
#for count, value in sorted(cache_missrate.items(), key=lambda item: item[1]): 
#    f_ptr.write(str(count)+": "+str(value)+"\n")   
#f_ptr.write("==================cache bandwidth per iter====================\n") 
#for count, value in sorted(cache_bandwidth.items(), key=lambda item: item[1],reverse=True): 
#    f_ptr.write(str(count)+": "+str(value)+"\n")
f_ptr.write("==================largest output frontier per pod in each iter====================\n") 
for count, value in sorted(largest_out.items(), key=lambda item: item[1],reverse=True): 
    f_ptr.write(str(count)+": "+str(value)+"\n")
f_ptr.write("==================output frontier in each iter====================\n")
for count, value in out_per_ite.items(): 
    f_ptr.write(str(count)+": "+str(value)+"\n")

f_ptr.write("==================total status====================\n") 
f_ptr.write("total instruction: "+str(aggr_inst)+"\n")
f_ptr.write("total abs cycle: "+str(abs_cyc)+"\n")
f_ptr.write("total stall cycle: "+str(aggr_stall)+"\n")
f_ptr.write("total IPC: "+str(aggr_inst/aggr_cyc)+"\n")
#f_ptr.write("stall percentage: "+str(aggr_stall/aggr_cyc)+"\n")
#f_ptr.write("dram load stall percentage: "+str(dram_stall/aggr_stall)+"\n")
#f_ptr.write("total traversed edges: "+str(traversed_edges)+"\n")
exec_time=abs_cyc/1500000
f_ptr.write("total execution time: "+str(exec_time)+" ms\n")
#f_ptr.write("throughput in MEPS: "+str(traversed_edges/1000/exec_time)+" MEPS\n")
f_ptr.write("total frontier passing size: "+str(out_total)+" \n")
f_ptr.write("frontier passing time: "+str(out_total*4/1024/1024/96)+" \n")


f_ptr.close()

print("total instruction:",aggr_inst)
print("total abs cycle:",abs_cyc)
print("IPC:",aggr_inst/aggr_cyc)
print("total stall cycle:", aggr_stall)
print("stall percentage:",aggr_stall/aggr_cyc)
print("largest out:", largest_out)





    
