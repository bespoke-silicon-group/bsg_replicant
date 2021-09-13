import pandas
import re
class StreamParameters(object):
    def __init__(self, filename):
        """
        Parse the experiment parameters from the filename
        """
        self._filename = filename
        m = re.search(r'(\d+)_table-words__(\d+)_threads-per-group__(\d+)_block-words-per-thread', self.filename)
        self._table_words = int(m.group(1))
        self._threads_per_group = int(m.group(2))
        self._block_words_per_thread = int(m.group(3))

    def updateDataFrame(self, df):
        """
        Adds parameters to data frame as a column
        """
        df["table_words"] = self._table_words
        df["threads_per_group"] = self._threads_per_group
        df["block_words_per_thread"] = self._block_words_per_thread
        # return the dataframe
        return df
    
    @property
    def filename(self):
        return self._filename

    @property
    def table_words(self):
        return self._table_words

    @property
    def threads_per_group(self):
        return self._threads_per_group
    
    @property
    def block_words_per_thread(self):
        return self._block_words_per_thread

    @property
    def parameters(self):
        """
        Return a list of parameters
        """
        return [
            'table_words',
            'threads_per_group',
            'block_words_per_thread',
        ]

TAIL = 0.25
