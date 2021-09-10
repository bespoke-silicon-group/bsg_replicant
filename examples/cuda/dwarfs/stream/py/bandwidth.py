import argparse
import pandas as pd
from tag import Tag

parser = argparse.ArgumentParser()
parser.add_argument('dramsim_tag_json')
parser.add_argument('output')

args = parser.parse_args()
data = pd.read_json(args.dramsim_tag_json)
data['tag_type']=data['tag'].map(lambda tag : Tag(tag).type)

# lop off the tail data
# this discounts cycles where < 25% of tiles are still running
lbound=data[data['tag_type'].str.match('kernel_start')]['num_cycles'].quantile(0.25)
ubound=data[data['tag_type'].str.match('kernel_end')]['num_cycles'].quantile(0.75)

# take our min and max cycle
min_cycle=data[data['num_cycles']>=lbound]['num_cycles'].min()
max_cycle=data[data['num_cycles']<=ubound]['num_cycles'].max()

print("start_cycle    = {:9d}".format(min_cycle))
print("end_cycle      = {:9d}".format(max_cycle))
print("elapsed_cycles = {:9d}".format((max_cycle - min_cycle)))

# just keep the min/max cycle
data = data[(data['num_cycles']==min_cycle)|(data['num_cycles']==max_cycle)]

num_channels=data['channel'].nunique()

data=data.groupby(['tag','channel']).sum().diff(num_channels).dropna()
data=data.reset_index()
data['read_bw:bytes/cycle']  = data['num_reads_done']*32/data['num_cycles']
data['write_bw:bytes/cycle'] = data['num_writes_done']*32/data['num_cycles']
data['page_hit_rate'] = (1-(data['num_act_cmds']/data['num_reads_done']))

# condense useful data
condensed = data[['channel'
                  ,'num_cycles'
                  ,'num_reads_done'
                  ,'num_writes_done'
                  ,'num_act_cmds'
                  ,'read_bw:bytes/cycle'
                  ,'write_bw:bytes/cycle'
                  ,'page_hit_rate']]
print(condensed)
condensed.to_csv(args.output,sep=',')

# # keep min/max cycles
# min_cycle = data['global_ctr'].min()
# max_cycle = data['global_ctr'].max()

# # prune
# data = data[(data['global_ctr']==min_cycle)|(data['global_ctr']==max_cycle)]
# data = data.groupby(['tag','time','global_ctr']).sum().diff().dropna()
# print(data[['dma_read_req']])

# outputstr = """
# {} DMA read  requests
# {} DMA write wequests
# {} cycles at 1GHz
# {} bytes/second read bandwidth
# {} bytes/second write bandwidth
# """

# print(outputstr.format(data['dma_read_req'].sum()
#                        , data['dma_write_req'].sum()
#                        , max_cycle-min_cycle
#                        , data['dma_read_req']))
