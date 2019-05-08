import os
import tarfile
from argparse import Action, ArgumentTypeError
class VersionAction(Action):
    def __call__(self, parser, namespace, versions, option_string=None, nargs=None):
        v = self.validate(versions[0])
        setattr(namespace, self.dest, v)

    def validate(self, v):
        ds = v.strip('rc').split('.')
        if len(ds) != 3 and [int(x) for x in ds]:
            raise ArgumentTypeError("Invalid Version ID: {0} must be three"
                                    "decimal numbers separated by '.'".format(v))
        return v.strip()
