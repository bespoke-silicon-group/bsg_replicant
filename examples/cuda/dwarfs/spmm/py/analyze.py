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
    
region_data = filtered_data.groupby(['time','tag','tag_type','tag_id']).sum().diff()
region_data['global_ctr']=region_data['global_ctr']/data.groupby(['x','y']).ngroups
region_data = region_data.reset_index()
region_data = region_data[region_data['tag_type'].str.match('end')]
region_data.to_csv('vcache.analyze.csv')

region_data['global_ctr_%'] = 100*region_data['global_ctr']/region_data['global_ctr'].sum()
summary = region_data[['tag_id','global_ctr','global_ctr_%']]
print(summary)

# solve_row_data = data[data['tag_id']=='solve_row']
# print(solve_row_data)
# print(solve_row_data.groupby(['time','global_ctr','tag','tag_type','tag_id']).sum().diff())

