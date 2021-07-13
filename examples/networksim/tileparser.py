#!env python3

import pandas as pd
import seaborn as sns
from enum import Enum

# Read the CSV into pandas
df = pd.read_csv("dpi_stats.csv")

# This code is basically copied from BSG Manycore...

# Create the ManycoreCoordinate class, a surprisingly useful wrapper
# for a tuple. Access the y and x fields using var.y and var.x
from collections import namedtuple
ManycoreCoordinate = namedtuple('ManycoreCoordinate', ['y', 'x'])

# CudaStatTag class 
# Is instantiated by a packet tag value that is recieved from a 
# bsg_cuda_print_stat(tag) insruction
# Breaks down the tag into (type, y, x, tg_id, tag>
# type of tag could be start, end, stat
# x,y are coordinates of the tile that triggered the print_stat instruciton
# tg_id is the tile group id of the tile that triggered the print_stat instruction
# Formatting for bsg_cuda_print_stat instructions
# Section                 stat type  -   y cord   -   x cord   -    tile group id   -        tag
# of bits                <----2----> -   <--6-->  -   <--6-->  -   <------14----->  -   <-----4----->
# Stat type value: {"Kernel Start":0, "Kernel End": 1, "Tag Start":2, "Tag End":3}

# The CudaStatTag class encapsulates the tag argument used by bsg_cuda_print_stat_*
# commands inside of bsg_manycore/software/bsg_manycore_lib/bsg_manycore.h.
# There are four commands:

#  bsg_cuda_print_stat_kernel_start() - Annotates the start of the kernel being profiled
#  bsg_cuda_print_stat_kernel_end()   - Annotates the end of the kernel being profiled
#  bsg_cuda_print_stat_start(tag)     - Annotates the start of a tagged section of the kernel being profiled
#  bsg_cuda_print_stat_end(tag)       - Annotates the end of a tagged section of the kernel being profiled

# Calls to bsg_cuda_print_stat_start(tag) and bsg_cuda_print_stat_kernel_start()
# must be called first be paired with a matching call to
# bsg_cuda_print_stat_end(tag) and bsg_cuda_print_stat_kernel_end().
class CudaStatTag:
    # These values are used by the manycore library in bsg_print_stat instructions
    # they are added to the tag value to determine the tile group that triggered the stat
    # and also the type of stat (stand-alone stat, start, or end)
    # the value of these paramters should match their counterpart inside 
    # bsg_manycore/software/bsg_manycore_lib/bsg_manycore.h
    _TAG_WIDTH   = 4
    _TAG_INDEX   = 0
    _TAG_MASK   = ((1 << _TAG_WIDTH) - 1)
    _TG_ID_WIDTH = 14
    _TG_ID_INDEX = _TAG_WIDTH + _TAG_INDEX
    _TG_ID_MASK = ((1 << _TG_ID_WIDTH) - 1)
    _X_WIDTH     = 6
    _X_MASK     = ((1 << _X_WIDTH) - 1)
    _X_INDEX     = _TG_ID_WIDTH + _TG_ID_INDEX
    _Y_WIDTH     = 6
    _Y_INDEX     = _X_WIDTH + _X_INDEX
    _Y_MASK     = ((1 << _Y_WIDTH) - 1)
    _TYPE_WIDTH  = 2
    _TYPE_INDEX  = _Y_WIDTH + _Y_INDEX
    _TYPE_MASK   = ((1 << _TYPE_WIDTH) - 1)

    class StatType(Enum):
        START = 0
        END = 1
        KERNEL_START   = 2
        KERNEL_END     = 3

    def __init__(self, tag):
        """ Initialize a CudaStatTag object """
        self.__s = tag;
        self.__type = self.StatType((self.__s >> self._TYPE_INDEX) & self._TYPE_MASK)

    @property
    def tag(self):
        """ Get the tag associated with this object """
        return ((self.__s >> self._TAG_INDEX) & self._TAG_MASK)

    @property
    def getTag(self):
        """ Get the tag associated with this object """
        if(self.__type == self.StatType.KERNEL_START or
           self.__type == self.StatType.KERNEL_END):
            return "Kernel"
        return ((self.__s >> self._TAG_INDEX) & self._TAG_MASK)

    @property 
    def tg_id(self):
        """ Get the Tile-Group ID associated with this object """
        return ((self.__s >> self._TG_ID_INDEX) & self._TG_ID_MASK)

    @property 
    def getTileGroupID(self):
        """ Get the Tile-Group ID associated with this object """
        return ((self.__s >> self._TG_ID_INDEX) & self._TG_ID_MASK)

    @property 
    def x(self):
        """ Get the X Coordinate associated with this object """
        return ((self.__s >> self._X_INDEX) & self._X_MASK)

    @property 
    def y(self):
        """ Get the Y Coordinate associated with this object """
        return ((self.__s >> self._Y_INDEX) & self._Y_MASK)

    @property 
    def getAction(self):
        """ Get the Action that this object defines"""
        return "Start" if self.__type in {self.StatType.KERNEL_START, self.StatType.START} else "End"

    @property 
    def statType(self):
        """ Get the StatType that this object defines"""
        return self.__type

    @property 
    def isStart(self):
        """ Return true if this object corresponds to a call to
        bsg_cuda_print_stat_start """
        return (self.__type == self.StatType.START)

    @property 
    def isEnd(self):
        """ Return true if this object corresponds to a call to
        bsg_cuda_print_stat_end """
        return (self.__type == self.StatType.END)

    @property 
    def isKernelStart(self):
        """ Return true if this object corresponds to a call to
        bsg_cuda_print_stat_kernel_start """
        return (self.__type == self.StatType.KERNEL_START)

    @property 
    def isKernelEnd(self):
        """ Return true if this object corresponds to a call to
        bsg_cuda_print_stat_kernel_end """
        return (self.__type == self.StatType.KERNEL_END)

# Parse raw_tag data using CudaStatTag
cst = df.Payload.map(CudaStatTag)
df = df.drop("Payload", axis="columns")

# Update the table with information parsed from CudaStatTag
df["Tile Group ID"] = cst.map(lambda e: e.getTileGroupID)
df["Tag"] = cst.map(lambda e: e.getTag)
df["Action"] = cst.map(lambda e: e.getAction)

hierarchy = ["Action", "Tile Group ID", "Tag", "Y", "X"]
sums = df.groupby(hierarchy).sum()

res = sums.loc["End"] - sums.loc["Start"]
res = res.rename(columns = {"Cycle": "Total Cycles"})
res["Idle"] = res["Idle"] - res["Stall RegID"]

cs = ["Total Cycles",
      "Stall Credit",
      "Stall !Ready",
      "Stall RegID",
      "Idle"]
      
des = {"Idle": "Endpoint ready, no packet sent by Tile.",
       "Total Cycles" : "Total cycles executed.",
       "Stall Credit": "Out of Credits (Packets in Flight). TX stalled (RX possible).",
       "Stall RegID": "Out of Register IDs (Packets out-of-order). TX stalled (RX possible)",
       "Stall !Ready": "Endpoint not ready (Congestion). TX stalled (RX possible).",
       }

tg = res.loc[0,0]
mni = tg["Total Cycles"].idxmin()[::-1]
mn = tg["Total Cycles"].min()

mxi = tg["Total Cycles"].idxmax()[::-1]
mx = tg["Total Cycles"].max()

with open("tile_execution.rpt", "w") as fd:
    fd.write("Execution Statistics:\n\n")
    fd.write(f"Min Cycles: {mn} @ {mni}\n")
    fd.write(f"Max Cycles: {mx} @ {mxi}\n\n")
    fd.write(f"Total Stall RegID : {tg['Stall RegID'].sum()}\n")
    fd.write(f"Total Stall Credit: {tg['Stall Credit'].sum()}\n")
    fd.write(f"Total Stall !Ready: {tg['Stall !Ready'].sum()}\n\n")

    for c in cs:
        fd.write(f"{c} -- {des[c]}\n")
        fd.write(tg.unstack()[c].to_string())
        fd.write("\n\n")


fig = sns.displot(tg["Total Cycles"])
fig.savefig("cycle_dist.png")

import matplotlib.pyplot as plt # for data visualization

plt.title("Total Cycles Heatmap")
fig = sns.heatmap(tg["Total Cycles"].unstack(), vmin=3000, vmax=7000)
fig.get_figure().savefig("cycle_heat.png")

fig.get_figure().clf()

plt.title("Congestion Stalls Heatmap")
fig = sns.heatmap(tg["Stall !Ready"].unstack(), vmin=0, vmax=2000)
fig.get_figure().savefig("congestion.png")

fig.get_figure().clf()

plt.title("Credit Stalls Heatmap")
fig = sns.heatmap(tg["Stall Credit"].unstack(), vmin=0, vmax=4000)
fig.get_figure().savefig("credits.png")

fig.get_figure().clf()
plt.title("Stalls Heatmap")
fig = sns.heatmap(tg["Total Cycles"].unstack() - tg["Idle"].unstack(), vmin=0, vmax=6000)
fig.get_figure().savefig("stalls.png")

