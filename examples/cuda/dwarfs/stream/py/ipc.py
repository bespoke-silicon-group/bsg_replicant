import argparse
import pandas as pd

# build argument parser
parser = argparse.ArgumentParser()
parser.add_argument('vcache_stats_csv')
parsre.add_argument('output')

# argument parsers
args = parser.parse_args()
data = pd.read_json(args.vcache_stats_csv)

# take the min/max cycles
min_cycle=data['global_ctr'].min()
max_cycle=data['global_ctr'].max()

# just keep the min/max cycle
data = data[(data['global_ctr']==min_cycle)|(data['global_ctr']==max_cycle)]

num_cores = data[['x','y']].nunique().product()

