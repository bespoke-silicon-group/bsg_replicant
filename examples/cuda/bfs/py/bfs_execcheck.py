#Process level parallelism for make profile execution
import glob
import subprocess as sp
import multiprocessing as mp
from os.path import exists as file_exists  

def work(path):
    """Defines the work unit on an input file"""
    logstats = path + "stats/manycore_stats.log"
    log_exists = file_exists(logstats)
    while not log_exists:
        sp.run(["make", "clean"],cwd=path,stdout=sp.DEVNULL,stderr=sp.STDOUT)
        sp.run(["make","profile.log"],cwd=path,stdout=sp.DEVNULL,stderr=sp.STDOUT)
        sp.run(["python","/work/shared/users/phd/zy383/HB_Cosim/Bladerunner_6.2.0/bsg_bladerunner/bsg_manycore/software/py/vanilla_parser/stats_parser.py", "--stats", "vanilla_stats.csv", "--vcache-stats", "vcache_stats.csv"],check=False,cwd=path)
        log_exists = file_exists(logstats)
    return 0

if __name__ == '__main__':
    #Specify files to be worked with typical shell syntax and glob module
    folder_pattern = './hollywood_graph*/'
    tasks = glob.glob(folder_pattern)

    count = 0
    new_task = []
    for path in tasks:
        log_exists = file_exists(path+"stats/manycore_stats.log")
        if not log_exists:
            print("log file not found in:",path)
            count += 1
            new_task.append(path)
    print("total unsuccessfull dir is: ",count)
    
    
    #Set up the parallel task pool to use all available processors
    count = mp.cpu_count()
    count_r = min(count,10)
    #pool = mp.Pool(processes=count_r)

    #Run the jobs
    #pool.map(work, new_task)


    
