#Process level parallelism for make profile execution
import glob
import subprocess as sp
import multiprocessing as mp
import argparse
from os.path import exists as file_exists  


parser = argparse.ArgumentParser()

parser.add_argument("--dirpattern", type=str, default="pokec", 
                        help="pattern of the working directories")
parser.add_argument("--dirpath", type=str, default="./", 
                        help="path of the working directories")  
args = parser.parse_args()

def work(path):
    """Defines the work unit on an input file"""
    #rstats = path + "vanilla_stats.csv"
    #vstats = path + "vcache_stats.csv"
    #r_exists = file_exists(rstats)
    #v_exists = file_exists(vstats) 
    logstats = path + "stats/manycore_stats.log"
    log_exists = file_exists(logstats)
    while not log_exists :
        sp.run(["make", "clean"],cwd=path,stdout=sp.DEVNULL,stderr=sp.STDOUT)
        sp.run(["make","profile.log"],cwd=path,stdout=sp.DEVNULL,stderr=sp.STDOUT)
        sp.run(["python","/work/global/zy383/Bladerunner6.4.0/bsg_manycore/software/py/vanilla_parser/stats_parser.py", "--stats", "vanilla_stats.csv", "--vcache-stats", "vcache_stats.csv"],check=False,cwd=path)
        log_exists = file_exists(logstats)
    return 0

if __name__ == '__main__':
    #Specify files to be worked with typical shell syntax and glob module
    
    folder_pattern = args.dirpath + args.dirpattern + '*'
    tasks = glob.glob(folder_pattern)
    
    #Set up the parallel task pool to use all available processors
    count = mp.cpu_count()
    count_r = min(count,10)
    pool = mp.Pool(processes=count_r)

    #Run the jobs
    pool.map(work, tasks)


    
