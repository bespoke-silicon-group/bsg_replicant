#!/usr/bin/python3

# build.py is a script that builds an Amazon EC2 AMI with BSG tools/IP
# directories installed and configured. It uses the boto3 library to interact
# with the AWS console. The script launches an instance, passes a UserData
# script, waits for the instance to stop itself, and then creates an AMI.
import boto3
import datetime
import time
import argparse 
import os
from functools import reduce
from CommitIDAction import CommitIDAction
from PathAction import PathAction
from AwsVerAction import AwsVerAction
from JsonFileAction import JsonFileAction

# Read Commit ID in the form of " -r bsg_f1@xxxx bsg_manycore@xxxx bsg_ip_cores@xxxx " ==> These IDs get passed to ./bootstrap.sh --> ../amiconfig/setup.sh --> ../amiconfig/clone_repositories.sh
parser = argparse.ArgumentParser(description='Build an AWS EC2 F1 FPGA Image')
parser.add_argument("BuildDirectory", action=PathAction, nargs=1,
                    help='Path to Git Repositories')
parser.add_argument('AwsVersion', action=AwsVerAction, nargs=1,
                    help='AWS Repository version, e.g. 1.4.7')
parser.add_argument('-u', action=JsonFileAction, nargs=1,
                    default={"FpgaImageGlobalId":"Not-Specified-During-AMI-Build"},
                    help='JSON File Path with "FpgaImageId" and "FpgaImageGlobalId" defined')
parser.add_argument('-r', action=CommitIDAction, nargs='+',
                    help='A string with a repo name and commit id: git_repo@commit_id')
parser.add_argument('-d', '--dryrun', action='store_const', const=True,
                    help='Process the arguments but do not launch an instance')

args = parser.parse_args()

# The timestamp is used in the instance name and the AMI name
timestamp = datetime.datetime.now().strftime('%Y%m%d-%H%M%S')
instance_name = timestamp + '_image_build'
ami_name = 'BSG AMI ' + timestamp
base_ami = 'ami-0d0bd43b0b6c54f6f'
# The instance type is used to build the image - it does not need to match the
# final instance type (e.g. an F1 instance type)
instance_type = 't3.small'

# Connect to AWS Servicesn
ec2 = boto3.resource('ec2')
cli = boto3.client('ec2')

# Create a "waiter" to wait on the "Stopped" state
waiter = cli.get_waiter('instance_stopped')

# Open Userdata (bootstrap.h) and pass it (change in file) the commit_ids given by Makefile in argparse
bootstrap_path = os.path.join(args.r["bsg_f1"]["path"], 
                              "scripts", "amibuild", "bootstrap.sh")

UserData = open(bootstrap_path,'r').read()
UserData = UserData.replace("$bsg_f1_commit_id", args.r["bsg_f1"]["commit"])
args.r.pop("bsg_f1")
deps = reduce(lambda d, ds: d + " " + ds, [r + " " + c["commit"] 
                                           for (r, c) in args.r.items()], "")

UserData = UserData.replace("$agfi", args.u["FpgaImageGlobalId"])
UserData = UserData.replace("$aws_ver", args.AwsVersion)
UserData = UserData.replace("$dependencies", deps)

if(args.dryrun):
    print(UserData)
    exit(1)

# Create and launch an instance
instance = ec2.create_instances(
    ImageId=base_ami,
    InstanceType=instance_type,
    KeyName='cad-xor',
    SecurityGroupIds=['bsg_sg_xor_uswest2'],
        UserData=UserData,
        MinCount=1,
    MaxCount=1,
    TagSpecifications=[{'ResourceType':'instance',
                         'Tags':[{'Key':'Name',
                                  'Value':instance_name}]}],
    BlockDeviceMappings=[
        {
            'DeviceName': '/dev/sda1',
            'Ebs': {
                'DeleteOnTermination': True,
                'VolumeSize': 150,
            }
        },
    ])[0]

print('Generated Instance: ' + instance.id);

# This is necessary to give the instance some time to be registered
time.sleep(10)

waiter.wait(
    InstanceIds=[
        instance.id,
    ],
    WaiterConfig={
        'Delay': 60,
        'MaxAttempts': 120
    }
)

print('Instance configuration completed')

# Finally, generate the AMI 
ami = cli.create_image(InstanceId=instance.id, Name=ami_name)
print('Creating AMI: ' + ami['ImageId'])
