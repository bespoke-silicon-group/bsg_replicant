import os
from argparse import Action, ArgumentTypeError
class AwsVerAction(Action):
    def __call__(self, parser, namespace, v, option_string=None):
        setattr(namespace, self.dest, self.validate(v[0]))

    def validate(self, v):
        if(v[0] is not "v"):
            raise ValueError("AWS Version must start with 'v'")

        if(len(v) != 6 and len(v) != 7):
            raise ValueError("AWS Version must be a 6 or 7 character string: " +
                             "v1.4.8 or v1.4.10")
        try:
            [maj, min, submin] = v.split(".")
        except ValueError:
            raise ValueError("AWS Version must have three numbers")
            
        return v
