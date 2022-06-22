#!/usr/bin/sh
#SBATCH -J block_11
#SBATCH -o /work/global/zz546/bsg_bladerunner/bsg_replicant/examples/cuda/pagerank/wikivote-merge/block_11/block_11.out
#SBATCH -e /work/global/zz546/bsg_bladerunner/bsg_replicant/examples/cuda/pagerank/wikivote-merge/block_11/block_11.err
#SBATCH -N 1
#SBATCH -n 1
#SBATCH --get-user-env
#SBATCH --mem=16GB
#SBATCH -t 72:00:00
#SBATCH --partition=cpu
cd /work/global/zz546/bsg_bladerunner/bsg_replicant/examples/cuda/pagerank/wikivote-merge/block_11
module load synopsys-2020/synopsys-vcs-R-2020.12
export BRG_BSG_BLADERUNNER_DIR=/work/global/zz546/bsg_bladerunner
export BSG_MACHINE=pod_X1Y1_ruche_X16Y8_hbm_one_pseudo_channel
export BSG_MACHINE_PATH=$BRG_BSG_BLADERUNNER_DIR/bsg_replicant/machines/$BSG_MACHINE
make pc-histogram.log > out.log 2>&1
make kernel.dis > kernel.txt 2>&1
