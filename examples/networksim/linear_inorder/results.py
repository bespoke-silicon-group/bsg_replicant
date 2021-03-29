#!/bin/env python3

import pandas as pd
import sys
import seaborn as sns
import matplotlib.pyplot as plt
from itertools import chain

df = pd.read_csv(sys.argv[1], sep="\s+")
df["Cycles"] = df.Cycles.map(int)

df = df.set_index(["Stripe Size (Words)", "Cache Response Elements", "Allocation"]).unstack()

df.columns = df.columns.droplevel(0)

ints = filter(str.isnumeric, df.columns.values)
ints = list(map(int, ints))
ints.sort()
ints = map(str, ints)
strs = filter(lambda x: not str.isnumeric(x), df.columns.values)


df = df[chain(strs, ints)]

df.to_csv(sys.argv[1] + ".csv")

plt.figure(figsize=(14, 16))

h = sns.heatmap(df, annot=True, fmt="3.0f", vmin = 5000, vmax=10000)
fig = h.get_figure()
fig.savefig(sys.argv[1] + ".png")
