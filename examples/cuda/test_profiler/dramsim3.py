#!/usr/bin/env python
# coding: utf-8

# In[1]:


import pandas as pd
import sys
import os
sys.path.append("/mnt/users/ssd2/homes/drichmond/Research/repositories/git/bsg_bladerunner.release/bsg_manycore/software/py/vanilla_parser/")
from CudaStatTag import CudaStatTag
import ManycoreCoordinate
from collections import Counter
import seaborn as sns

bus_width = 128
burst_len = 8


# In[2]:


df = pd.read_json('dramsim3.tag.json')
df['raw_tag'] = df.tag


# In[3]:


df = df.apply(lambda col: col.map(Counter) if(col.dtype == object) else col)
df = pd.concat([df, CudaStatTag.parse_raw_tag(df)], axis='columns')
df = df.drop(['tag', 'raw_tag', 'epoch_num'], axis='columns')
df = df.drop(filter(lambda s: s.startswith("average_"), df.columns), axis='columns')


# In[4]:


# Assign Tile-Tag Iterations
hierarchy = ["Action", "Tag", "Tile Coordinate (Y,X)", "channel"]
iters = df.groupby(hierarchy).cumcount()
df["Tile-Tag Iteration"] = iters


# In[5]:


tagdf = df.drop(["Tile Group ID"], axis='columns')
channeldf = df.drop(["Tile Group ID", "Tile Coordinate (Y,X)"], axis='columns')


# In[17]:


hierarchy = ["Action", "Tag", "channel", "Tile-Tag Iteration"]
channels = channeldf.set_index(hierarchy)
starts = channels.loc["Start"]
ends = channels.loc["End"]

starts = starts.groupby(["Tag", "channel", "Tile-Tag Iteration"]).first()
ends = ends.groupby(["Tag", "channel", "Tile-Tag Iteration"]).last()
cdf = (ends - starts).groupby(["Tag", "channel"]).apply(lambda x: x.sum()) # Just groupby.sum removes Counters
tdf = (ends - starts).groupby(["Tag"]).apply(lambda x: x.sum())

# post-Processing
# Compute pJ/bit using the same method that DRAMSim3 does
cdf["pJ/bit"] = cdf.total_energy / ((cdf.num_reads_done + cdf.num_writes_done) * bus_width * burst_len)
tdf["pJ/bit"] = tdf.total_energy / ((tdf.num_reads_done + tdf.num_writes_done) * bus_width * burst_len)
cdf["Read Hit Rate"] = cdf.num_read_row_hits / cdf.num_read_cmds
tdf["Read Hit Rate"] = tdf.num_read_row_hits / tdf.num_read_cmds
cdf["Write Hit Rate"] = cdf.num_write_row_hits / cdf.num_write_cmds
tdf["Write Hit Rate"] = tdf.num_write_row_hits / tdf.num_write_cmds


# In[20]:


tdf[[c for c in tdf.columns if "_latency" not in c]].columns


# In[ ]:





# In[80]:


df.read_latency


# In[ ]:


bus_width = 128
burst_len = 8

