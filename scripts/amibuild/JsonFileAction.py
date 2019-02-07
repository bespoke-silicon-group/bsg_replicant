import os
import json
from argparse import Action, ArgumentTypeError
class JsonFileAction(Action):
    def __call__(self, parser, namespace, path, option_string=None, nargs=None):
        d = self.validate(path[0])

        setattr(namespace, self.dest, d)

    def validate(self, p):
        if not os.path.isfile(p):
            raise FileNotFoundError("Path to JSON File: {0} is not a valid path".format(p))

        with open(p, 'r') as f:
            d = json.loads(f.read())
            
        try:
            d["FpgaImageGlobalId"]
        except KeyError:
            KeyError("FpgaImageGlobalId key not found in file {}".format(p))
            
        return d

        
