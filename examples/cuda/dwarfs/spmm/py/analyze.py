import pandas as pd
import sys
from tag import Tag

data = pd.read_csv(sys.argv[1])
data['tag_type']=data['tag'].apply(lambda tag : Tag(tag).type)
data['tag_id']=data['tag'].apply(lambda tag : Tag(tag).tag_id)

print(data)

filtered_data = pd.DataFrame()
regions = ['solve_row', 'compute_offset', 'copy_results']
for region in regions:
    # filter only this region
    rdata = data[data['tag_id'].str.match(region)]
    # get the start and end dataa
    rdata_starts = rdata[rdata['tag_type'].str.match('start')]
    rdata_ends   = rdata[rdata['tag_type'].str.match('end')]
    # get the min cycle for the start
    min_cycle=rdata_starts['global_ctr'].min()
    # get the max cycle for the end
    max_cycle=rdata_ends['global_ctr'].max()
    # keep only the rows matching this cycle
    filtered = rdata[((rdata['tag_type'].str.match('start')) & (rdata['global_ctr']==min_cycle)
                      |(rdata['tag_type'].str.match('end')) & (rdata['global_ctr']==max_cycle))]
    # append
    filtered_data = filtered_data.append(filtered)
    print(filtered_data)
    print(filtered_data['miss_ld'])
    
region_data = filtered_data.groupby(['time','tag','tag_type','tag_id']).sum().diff()
print(region_data)
#region_data['global_ctr']=region_data['global_ctr']/data.groupby(['x','y']).ngroups
region_data = region_data.reset_index()
region_data = region_data[region_data['tag_type'].str.match('end')]
region_data.to_csv('vcore.analyze.csv')

region_data['global_ctr_%'] = 100*region_data['global_ctr']/region_data['global_ctr'].sum()
region_data['instr_per_cycle']=region_data['instr_total']/region_data['global_ctr']

summary = region_data[['tag_id','global_ctr','global_ctr_%']]
print("{}\n".format(summary))


summary = region_data[['tag_id','global_ctr','instr_per_cycle']]
print("{}\n".format(summary))

stall_fields = [f for f in region_data.columns.to_list() if 'stall_' in f]

stall_data = region_data[['tag_id']+stall_fields]
stall_data['stall_total']=stall_data.iloc[:,1:-1].sum(axis=1)

print("{}\n".format(stall_data))
