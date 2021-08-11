import pandas
import re
class StrideParameters(object):
    def __init__(self, filename):
        """
        Parse the experiment parameters from the filename
        """
        self._filename = filename
        m = re.search(r'(\d+)_table-words__(\d+)_loads-per-core__(\d+)_ilp__(\d+)_stride-words__(\d+)_tile-x__(\d+)_tile-y'
                      , self.filename)
        
        self._table_words = int(m.group(1))
        self._loads_per_core = int(m.group(2))
        self._ilp = int(m.group(3))
        self._stride_words = int(m.group(4))
        self._tile_x = int(m.group(5))
        self._tile_y = int(m.group(6))

    def updateDataFrame(self, df):
        """
        Adds parameters to data frame as a column
        """
        df["table_words"] = self._table_words
        df["loads_per_core"] = self._loads_per_core
        df["ilp"] = self._ilp
        df["stride_words"] = self._stride_words
        df["tile_x"] = self._tile_x
        df["tile_y"] = self._tile_y
        # return the dataframe
        return df
    
    @property
    def filename(self):
        return self._filename

    @property
    def table_words(self):
        return self._table_words

    @property
    def loads_per_core(self):
        return self._loads_per_core

    @property
    def ilp(self):
        return self._ilp

    @property
    def stride_words(self):
        return self._stride_words

    @property
    def tile_x(self):
        return self._tile_x

    @property
    def tile_y(self):
        return self._tile_y

    @property
    def parameters(self):
        """
        Return a list of parameters
        """
        return [
            'table_words'
            ,'loads_per_core'
            ,'ilp'
            ,'stride_words'
            ,'tile_x'
            ,'tile_y'
        ]
