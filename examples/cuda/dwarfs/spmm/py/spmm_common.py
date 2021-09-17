import pandas
import re
class SPMMParameters(object):
    def __init__(self, filename):
        """
        Parse the experiment parameters from the filename
        """
        self._filename = filename
        m = re.search(r'()_input__(\d+)_tx__(\d+)_ty', self.filename)
        self._input = m.group(1)
        self._tx = int(m.group(2))
        self._ty = int(m.group(3))

    def updateDataFrame(self, df):
        """
        Adds parameters to data frame as a column
        """
        df["input"] = self._input
        df["tx"] = self._tx
        df["ty"] = self._ty
        # return the dataframe
        return df
    
    @property
    def filename(self):
        return self._filename

    @property
    def input(self): return self._input
    @property
    def tx(self): return self._tx
    @property
    def ty(self): return self._ty
    @property
    def parameters(self):
        """
        Return a list of parameters
        """
        return [
            "input",
            "tx",
            "ty",
        ]
