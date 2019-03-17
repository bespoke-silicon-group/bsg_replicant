import os
import json
from argparse import Action, ArgumentTypeError
import boto3
class AfiAction(Action):
    def __call__(self, parser, namespace, agfi, option_string=None, nargs=None):
        d = self.validate(agfi[0])

        setattr(namespace, self.dest, d)

    def validate(self, agfi):
        ec2 = boto3.resource('ec2')
        cli = boto3.client('ec2')
        rsp = cli.describe_fpga_images(
            DryRun=False,
            FpgaImageIds=['afi-044f5fc0792dfe575'],
            Filters=[
                {
            'Name': 'fpga-image-global-id',
                    'Values': [
                        'agfi-087dad34c50a15366',
                    ]
                },
            ],
            MaxResults=5
        )['FpgaImages']

        if(len(rsp) == 0):
            raise ValueError('Error! AFI {} not found'.format(agfi))

        return {'AGFI' : rsp[0]['FpgaImageGlobalId'],
                'AFI'  : rsp[0]['FpgaImageId']}


