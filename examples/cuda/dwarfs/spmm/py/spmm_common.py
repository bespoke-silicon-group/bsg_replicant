import pandas
import re

class SPMMParametersBase(object):
    def __init__(self, filename):
        """
        Parse the experiment parameters from the filename
        """
        self._filename = filename
        m = re.search(r'(.*?)_input', self.filename)
        self._input = m.group(1)

    def updateDataFrame(self, df):
        """
        Adds parameters to data frame as a column
        """
        df["input"] = self.input
        # return the dataframe
        return df
    
    @property
    def filename(self):
        return self._filename

    @property
    def input(self): return self._input

    @property
    def parameters(self):
        """
        Return a list of parameters
        """
        return ["input"]

class SPMMParameters(SPMMParametersBase):
    def __init__(self, filename):
        super().__init__(filename)
        m = re.search(r'(\d+)_tx__(\d+)_ty', self.filename)
        self._tx = int(m.group(1))
        self._ty = int(m.group(2))

    def updateDataFrame(self, df):
        df = super().updateDataFrame(df)
        df['tx'] = self.tx
        df['ty'] = self.ty
        return df
    
    @property
    def tx(self): return self._tx
    
    @property
    def ty(self): return self._ty

    @property
    def parameters(self):
        return super().parameters + ['tx','ty']
    

class SPMMSolveRowParameters(SPMMParametersBase):
    def __init__(self, filename):
        super().__init__(filename)
        m = re.search(r'(\d+)_row__(.*?)_algorithm', self.filename)
        self.row = int(m.group(1))
        self.alg = m.group(2)

    def updateDataFrame(self, df):
        df = super().updateDataFrame(df)
        df['row'] = self.row
        df['algorithm'] = self.alg
        return df

    @property
    def parameters(self):
        return super().parameters + ['row','algorithm']

class HashTableParameters(SPMMParametersBase):
    def __init__(self, filename):
        super().__init__(filename)
        m = re.search(r'(\d+)_row__(\d+)_dmem-words__(\d+)_hash-table-words', self.filename)
        self.row = int(m.group(1))
        self.dmem = int(m.group(2))
        self.tblsz = int(m.group(3))

    def updateDataFrame(self, df):
        df = super().updateDataFrame(df)
        df['row'] = self.row
        df['dmem-words'] = self.dmem
        df['table-words'] = self.tblsz
        return df

    @property
    def parameters(self):
        return super().parameters + ['row','dmem-words', 'table-words']
    


class SPMMAbrevParameters(SPMMParametersBase):
    def __init__(self, filename):
        super().__init__(filename)
        m = re.search(r'(\d+)_row-base__(\d+)_rows__(yes|no)_opt__(yes|no)_parallel'
                      , self.filename)
        self.row = int(m.group(1))
        self.row_base = int(m.group(2))
        self.opt = m.group(3)
        self.parallel = m.group(4)

    def updateDataFrame(self, df):
        df = super().updateDataFrame(df)
        df['row'] = self.row
        df['row-base'] = self.row_base
        df['opt'] = self.opt
        df['parallel'] = self.parallel
        return df

    @property
    def parameters(self):
        return super().parameters + ['row'
                                     ,'row-base'
                                     , 'opt'
                                     , 'parallel']
    
    
