#!/usr/bin/env python3
import pandas as pd
import re
import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt
def idx_map(s):
    i = 0 if "north" in s else 16
    i += int(re.findall("\[\d+\]",s)[-1].strip("[]"))
    return i

df = pd.read_csv("vcache_operation_trace.csv")
df["idx"] = df.vcache.map(idx_map)
df["op"] = (df.operation == "ld_lw").map(int)
df["stall"] = (df.operation == "stall_rsp").map(int)
df["idle"] = (df.operation == "idle").map(int)
dfs = df.groupby("cycle").sum()
dfs.op = np.convolve([1.0] * 100, dfs.op/32.0, "same")/100.0
dfs.stall = np.convolve([1.0] * 100, dfs.stall/32.0, "same")/100.0
dfs.idle = np.convolve([1.0] * 100, dfs.idle/32.0, "same")/100.0
dfs.index = dfs.index - dfs.index.min()

ax = sns.lineplot(data=dfs[["op", "idle", "stall"]])
_ = ax.set(ylabel="Percent of Cycles (%)", ylim=(0,1.0))
fig = ax.get_figure()
fig.savefig("cache_utilization.png")
