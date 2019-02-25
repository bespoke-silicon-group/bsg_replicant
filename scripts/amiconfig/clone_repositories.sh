# TODO Check odd number of args

# Clone the aws-fpga repository
git clone https://github.com/aws/aws-fpga.git $AWS_FPGA_REPO_DIR
cd $AWS_REPO_DIR
git checkout $1
cd ~
shift

while (( "$#" )); do
    git clone https://bitbucket.org/taylor-bsg/$1.git
    cd $1
    git checkout $2
    cd ~

    shift
    shift
done
