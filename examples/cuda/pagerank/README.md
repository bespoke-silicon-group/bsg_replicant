The pagerank scale up simulation flow
===================

Simulation
----------------

1. Prepare the dataset, download gapbs and install, following [here](https://github.com/sbeamer/gapbs)

2. Using the ./converter to convert the .mtx dataset to .el. For example: `./convert -f xxx.mtx -e xxx.el`

3. Replace the bsg_replicant with graphit_pagerank branch which can be found [here](https://github.com/bespoke-silicon-group/bsg_replicant/tree/pagerank-new-machine)

4. `cd bsg_bladerunner/bsg_replicant/examples/cuda/pagerank`

5. Copy the roadcentral-scale-up folder to your test folder, say ljournal-scale-up

6. `cd ljournal-scale-up`

7. Change the `C_ARGS ?= $(BSG_MANYCORE_KERNELS) -g /home/zz546/gapbs/suitesparse_roadcentral.el` inside Makefile to the path of your dataset.

8. `python run-all-hb.py`

Collect the data
----------------

1. Replace the bsg_bladerunner/bsg_manycore/software/py/vanilla_stats/stats_parser.py with the one under the pagerank folder.

2. Run `python collect-all-data.py`

