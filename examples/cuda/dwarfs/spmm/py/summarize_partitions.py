import pandas as pd
import sys

data = pd.DataFrame()

partition_stats_files = sys.argv[2:]
for stats_file in partition_stats_files:
    d = pd.read_csv(stats_file)
    rmax = d['C_row'].max()
    rmin = d['C_row'].min()
    fields = [
        'A_nonzeros'
        ,'B_nonzeros'
        ,'C_nonzeros'
        ,'ops_fp'
        ,'ops_fmul'
        ,'ops_fadd'
        ,'lookups'
        ,'insertions'
        ,'updates'
    ]
    sum_d = d[fields].sum()
    min_d = d[fields].min()
    max_d = d[fields].max()
    dic = {
        'Partition'   : [stats_file]
        ,'C_row_min'  : [rmin]
        ,'C_row_max'  : [rmax]        
    }
    dic.update({f + '_sum' : [sum_d[f]] for f in fields})
    dic.update({f + '_min' : [min_d[f]] for f in fields})
    dic.update({f + '_max' : [max_d[f]] for f in fields})
    data = data.append(pd.DataFrame(dic))

data.to_csv(sys.argv[1])
