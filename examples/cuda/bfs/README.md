## BFS simulation insturctions ##


### Step 1: Prepare graphs in CSR or CSC formats ###   
We need to prepare CSR and CSC formats of the input graphs for PUSH and PULL directions.    
The scripts under ./py/mat_to_CSR.py and ./py/mat_to_CSC.py are used to convert the graph from .mtx format to a specific txt format
(with row/col pointers and row/col indices in different txt files).   
The generated CSR/CSC files should be put under path ./inputs/***GRAPH_NAME***/CSR/ or ./inputs/***GRAPH_NAME***/CSC/. Here, ***GRAPH_NAME*** is the name
of the graph used as an argument sent to the mat_to_CSR/CSC.py script.
As an example, the wiki-Vote dataset in txt format has already been put under the right path. For other datasets, first download the mtx format to ./input/MTX and then run mat_to_CSC.py and mat_to_CSR.py to get the corresponding txt files. 

### Step 2: Create seperate working directories for each itearation ###
We do the co-simulation of BFS on a per-iteration-per-pod basis, to leverage more parallelism. To achieve this, we have to first 
create different derectories in which simulations are run in parallel for different iterations on different pods. 

**First, write the config.mk**    
This configure file includes parameters to generate working directories.  
__TEST_GRAPH_TYPE__ should be set the same as ***GRAPH_NAME*** in the previous step.  
__MAX_POD__ defines the max pod ID which is 0-based (e.g., when set to 63, total number of pods is 64)  
__ITE_START__ and __ITE_END__ specify the scope of iterations covered by the generated working directories (one directory for one iteration only) 
Wiki-Vote is used as an example in the default config.mk. The root is set to 4668. There are total 6 iterations, with each iteration running in 64 pods. 

**Second, run ``make tests_dir``**   
Working directories will be generated in this step. By default, 64x6 directories will be generated for wiki-Vote.

### Step 3: Run parallel simulation ###
Run ``python py/bfs_parallelexec.py``  
*dirpattern* : the directory pattern, generally set as ***GRAPH_NAME***, defualt is wiki-Vote  

### Step 4: Collect data ###
Run ``python py/bfs_datacollect_1tag.py``  
*dirpattern* same as bfs_parallelexec.py    
*logpath* : path and name of the result log file  
*ite_start* : the start iteration ID to be collected from  
*ite_num* : number of iterations to be collected from the ite_start ID
wiki-Vote is used as default

