from vcache_utils import *
import pandas as pd
import sys
from mutex_common import MutexParameters

class MutexVCacheStats(VCacheStats):
    def _subclass_init_add_group_by_fields(self):
        self._parameters = MutexParameters(self.filename)
        self._parameters.updateDataFrame(self._data)
        self._group_by_fields += self._parameters.parameters
        return

data = pd.DataFrame()
for filename in sys.argv[1:]:
    data = data.append(MutexVCacheStats(filename).diffed_data)

data.to_csv('vcache.summary.csv')
