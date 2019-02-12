# build.py is a script that builds an Amazon EC2 AMI with BSG tools/IP
# directories installed and configured. It uses the boto3 library to interact
# with the AWS console. The script launches an instance, passes a UserData
# script, waits for the instance to stop itself, and then creates an AMI.
import boto3
import datetime
import time

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

# Create and launch an instance
instance = ec2.create_instances(
    ImageId=base_ami,
    InstanceType=instance_type,
    KeyName='cad-xor',
    SecurityGroupIds=['bsg_sg_xor_uswest2'],
    UserData=open('bootstrap.sh','r').read(),
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


