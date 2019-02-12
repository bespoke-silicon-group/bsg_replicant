# Build Scripts

The scripts in this directory build AMIs from a host server. These scripts are
split into two parts: 

## build.py

build.py is responsible for the overall process of building an AMI.The first
script launches an instance, loads the bootstrap script, waits for the image to
stop itself, and then generates the AMI from that stopped instance.

The generated AMI is named with a unique timestamp value in the name: 
"BSG AMI <Year><Month><Day>-<Hour><Minute><Second>". (This may change in the
future to include the git commit ID)

build.py uses the [boto3](https://boto3.amazonaws.com/v1/documentation/api/latest/index.html)
library to interact with the AWS Console.

## bootstrap.sh

bootstrap.sh is a UserData shell script that is run by the instance on first
boot. It is passed as a string and run with root permissions.

For more information about UserData can be found
[here](https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/user-data.html).



