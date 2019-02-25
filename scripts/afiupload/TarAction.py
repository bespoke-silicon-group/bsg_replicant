import os
import tarfile
from argparse import Action, ArgumentTypeError
class TarAction(Action):
    def __call__(self, parser, namespace, paths, option_string=None, nargs=None):
        p = self.validate(paths[0])

        setattr(namespace, self.dest, p)

    def validate(self, p):
        if not os.path.exists(p):
            raise ArgumentTypeError("Path to Tar file: {0} is not a valid path".format(p))

        try: 
            tarfile.open(p)
        except IOError:
            raise ArgumentTypeError("Path to Tar file: {0} is not a valid tar file".format(p))
        return p
