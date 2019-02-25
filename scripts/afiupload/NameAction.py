import os
import tarfile
from argparse import Action, ArgumentTypeError
class NameAction(Action):
    def __call__(self, parser, namespace, names, option_string=None, nargs=None):
        n = self.validate(names[0])
        setattr(namespace, self.dest, n)

    def validate(self, n):
        if len(n.split()) > 1 and n.strip().isalnum():
            raise ArgumentTypeError("Invalid Image Name: {0} must be an alphanumeric string".format(n))
        return n.strip()
