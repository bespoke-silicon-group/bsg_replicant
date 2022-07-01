import pandas as pd

bytes_per_atom = 32
class Tag(object):
    # constants
    TAG_TYPE_KERNEL_START = 2
    TAG_TYPE_KERNEL_END   = 3
    # constructor
    def __init__(self, raw):
        self.raw = raw
    # members
    # tag
    @property
    def tag(self):
        return self.raw & 0x0000000F
    # tile group
    @property
    def tg(self):
        return (self.raw >> 4)  & 0x00003FFF
    # x
    @property
    def x(self):
        return (self.raw >> 18) & 0x0000003F
    # y
    @property
    def y(self):
        return (self.raw >> 24) & 0x0000003F

    # type
    @property
    def type(self):
        return (self.raw >> 30) & 0x00000003

    # id
    @property
    def id(self):
        return (self.raw & 0xF)

ids = {
    0 : 'kernel'
}

types = {
    2 : 'kernel_start',
    3 : 'kernel_stop',
}

full_data = pd.read_json('dramsim3.tag.json')
full_data['tag_id'] = full_data['tag'].apply(lambda x : ids[Tag(x).id])
full_data['tag_type'] = full_data['tag'].apply(lambda x : types[Tag(x).type])

data = {}
for k,v in ids.items():
    data[v] = full_data[full_data['tag_id']==v]

for kernel, dataframe in data.items():
    print(kernel)
    start_tags = dataframe[dataframe['tag_type']=='kernel_start']
    stop_tags  = dataframe[dataframe['tag_type']=='kernel_stop']
    start_time_excl = start_tags['num_cycles'].min()
    stop_time_excl  = stop_tags['num_cycles'].max()
    start_data_excl = start_tags[start_tags['num_cycles']==start_time_excl]

    stop_data_excl = stop_tags[stop_tags['num_cycles']==stop_time_excl]    
    time_excl = (stop_time_excl-start_time_excl)
    num_reads_excl = stop_data_excl['num_reads_done'].max()-start_data_excl['num_reads_done'].min()
    num_writes_excl = stop_data_excl['num_writes_done'].max()-stop_data_excl['num_writes_done'].min()
    read_bw_excl = (num_reads_excl*bytes_per_atom/time_excl)
    write_bw_excl = (num_writes_excl*bytes_per_atom/time_excl)
    active_start_excl = start_data_excl['rank_active_cycles'].iloc[0]['0']
    active_stop_excl  =  stop_data_excl['rank_active_cycles'].iloc[0]['0']
    active_excl = active_stop_excl-active_start_excl
    active_perc_excl = 100*active_excl/time_excl
    
    start_time_incl  = start_tags['num_cycles'].max()
    stop_time_incl   = stop_tags['num_cycles'].min()
    start_data_incl = start_tags[start_tags['num_cycles']==start_time_incl]
    stop_data_incl = stop_tags[stop_tags['num_cycles']==stop_time_incl]
    time_incl = (stop_time_incl-start_time_incl)
    num_reads_incl = stop_data_incl['num_reads_done'].max()-start_data_incl['num_reads_done'].min()
    num_writes_incl = stop_data_incl['num_writes_done'].max()-stop_data_incl['num_writes_done'].min()
    read_bw_incl = (num_reads_incl*bytes_per_atom/time_incl)
    write_bw_incl = (num_writes_incl*bytes_per_atom/time_incl)
    active_start_incl = start_data_incl['rank_active_cycles'].iloc[0]['0']
    active_stop_incl  =  stop_data_incl['rank_active_cycles'].iloc[0]['0']
    active_incl = active_stop_incl-active_start_incl
    active_perc_incl = 100*active_incl/time_incl
    
    print("Kernel: %s" % kernel)
    print("Exclusive Read  BW: {} GB/second".format(read_bw_excl))
    print("Inclusive Read  BW: {} GB/second".format(read_bw_incl))
    print("Exclusive Write BW: {} GB/second".format(write_bw_excl))
    print("Inclusive Write BW: {} GB/second".format(write_bw_incl))
    print("Active % Inclusive: {} %".format(active_perc_incl))
    print("Active % Exclusive: {} %".format(active_perc_excl))
