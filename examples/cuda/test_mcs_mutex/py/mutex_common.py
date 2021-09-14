import re

class MutexParameters(object):
    REGEX = r'(mcs|simple)_lock-type__(\d+)_tx__(\d+)_ty__(\d+)_critical-region-length__(\d+)_non-critical-region-length__(\d+)_iters'
    def __init__(self, filename):
        m = re.search(self.REGEX,filename)
        self._lock_type = m.group(1)
        self._tx = int(m.group(2))
        self._ty = int(m.group(3))
        self._critical_region_length = int(m.group(4))
        self._non_critical_region_length = int(m.group(5))
        self._iters = int(m.group(6))

    @property
    def lock_type(self): return self._lock_type
    @property
    def tx(self): return self._tx
    @property
    def ty(self): return self._ty
    @property
    def crl(self): return self._critical_region_length
    @property
    def ncrl(self): return self._non_critical_region_length
    @property
    def iters(self): return self._iters

    def updateDataFrame(self, df):
        """
        Adds parameters to data frame as a column
        """
        df['lock_type'] = self.lock_type
        df['tx'] = self.tx
        df['ty'] = self.ty
        df['crl'] = self.crl
        df['ncrl'] = self.ncrl
        df['iters'] = self.iters
        # return the dataframe
        return df

    @property
    def parameters(self):
        """
        Return a list of parameters
        """
        return [
            'lock_type',
            'tx',
            'ty',
            'crl',
            'ncrl',
            'iters',
        ]
