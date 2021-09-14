import pandas as pd
import sys

vcache_csv = sys.argv[1]
iterations = int(sys.argv[2])
threads = int(sys.argv[3])

data = pd.read_csv(vcache_csv)
nvcache = data['vcache'].nunique()

data = data.groupby(['tag','vcache']).sum().diff(nvcache).dropna()

print(data[['global_ctr','instr_ld']])
print(data[['global_ctr','instr_st']])
print(data[['global_ctr','instr_amoswap']])

