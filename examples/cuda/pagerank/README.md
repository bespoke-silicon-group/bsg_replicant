
# Running pagerank examples
1. Code base structure:\\
   graphit: contains the different versions of interfaces that handle the format of input graphs on the host, including different partitioning stragies. \\
   kernel: contains the kernels using different optimization strategies on HB Manycore.\\
   other foloders: constains the host code and running scripts of different optimization strategies.\\
2. Get the pagerank branch from graphit by running `make checkout_graphit` using any Makefile inside the provided examples.
3. Input: the input graph of pagerank is the edge list format (.el) which can be converted from matrix market format (.mtx). The converter inside [gapbs]{https://github.com/sbeamer/gapbs} can be used to convert .mtx file to .el file.
4. Running on servers without clusters setting up, use `python run-all-hb.py` command. Remember to change the `BRG_BSG_BLADERUNNER_DIR` path to your own path in the file. Also by setting up `parallel_num` in `run-all-hb.py` file, you are able to control the parallel simulation jobs.
5. We provide the several examples of running pokec, roadca and wikivote, note that earch example has their own host code and kernel code. To change the dataset, you need to convert the .mtx file to edgelist format (.el) and set the path of input through Makefile in the example folder.
6. If you modify the name of the host code file, remember to update the name in the script file and Makefile. 
7. If you modify the name of the kernel code file, remember to update the name in the Makefile.
8. Current example includes:
   - Partition the graph in blocking manner to different pods.
   - Partition the graph in cyclic manner to different pods.
   - Kernel fusion.
   - Multi-unrolling factors.

 

