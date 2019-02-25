import os
from argparse import Action, ArgumentTypeError
class PathAction(Action):
    def __call__(self, parser, namespace, d, option_string=None):
        setattr(namespace, self.dest, self.validate(d[0]))

    def validate(self, d):
        if not os.path.isdir(d):
            raise ArgumentTypeError("Build Directory: {0} is not a valid path".format(d))
        return os.path.abspath(d)
