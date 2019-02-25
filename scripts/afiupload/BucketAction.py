import os
import boto3
from argparse import Action, ArgumentTypeError
from botocore.client import ClientError

class BucketAction(Action):
    def __call__(self, parser, namespace, buckets, option_string=None, nargs=None):
        b = self.validate(buckets[0])
        setattr(namespace, self.dest, b)

    def validate(self, b):
        s3 = boto3.resource('s3')
        s3cli = boto3.client('s3')

        if len(b.split()) > 1 or not b.strip().isalnum() or any(c.isupper() for c in b):
            raise ArgumentTypeError("Invalid Bucket Name: {0} must be an alphanumeric string with no upper-case letters".format(b))

        try:
            s3.meta.client.head_bucket(Bucket=b)
        except ClientError:
            raise ArgumentTypeError("Bucket {} not found").format(b)

        return s3.Bucket(b)


