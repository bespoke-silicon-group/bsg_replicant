import sys
import pandas as pd
import seaborn
import numpy as np
import argparse
from matplotlib import pyplot as plt

parser = argparse.ArgumentParser()
parser.add_argument('csv')
parser.add_argument('output')
parser.add_argument('--title', type=str, default='NNZ CDF')
args = parser.parse_args()

data_filename = args.csv
data = pd.read_csv(data_filename)

seaborn.ecdfplot(data, x='nnz')
plt.grid()
plt.yticks(np.arange(0,1.1,step=0.1))
xstep=(data['nnz'].max()-data['nnz'].min())/10
plt.xticks(np.arange(data['nnz'].min()
                     ,data['nnz'].max()+xstep
                     ,step=xstep))
plt.title(args.title)
plt.savefig(args.output, format='pdf')

seaborn.displot(data, x='nnz')
plt.grid()
plt.title(args.title)
plt.savefig(args.output.replace('cdfplot','displot'), format='pdf')
