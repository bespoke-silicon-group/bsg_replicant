#!/usr/bin/python

import argparse

MC_VERSION_ID = 0x00200200
COMPILATION_DATE = 0x03272019
DESCRIPTION_PTR = 0
NETWORK_ADDR_WIDTH = 28
NETWORK_DATA_WIDTH = 32
HOST_INTERFACE_COORD_X = 3
HOST_INTERFACE_COORD_Y = 0
NETWORK_DIMENSION_X = 4
NETWORK_DIMENSION_Y = 5
VCACHE_ASSOCIATIVITY=2
VCACHE_SETS=64
VCACHE_BLOCK_SIZE_IN_WORDS=16
VCACHE_STRIPE_SIZE_IN_WORDS=16

REPOS = ["bsg_manycore", "bsg_f1", "bsg_ip_cores"]

class ReleaseRepoAction(argparse.Action):
        def __call__(self, parser, namespace, repo_at_commit_ids, option_string=None):
            for release in repo_at_commit_ids:
                repo, commit = release.split("@")
                if repo not in REPOS:
                    raise ValueError("{} is not a valid repository!".format(repo))
                setattr(namespace, repo, int(commit, 16))

def parse_args():
    parser = argparse.ArgumentParser(description="Create FPGA ROM parameters")
    parser.add_argument("--network-addr-width", default=NETWORK_ADDR_WIDTH, type=int,
                        help="Bitwidth of network address bus")
    parser.add_argument("--network-data-width", default=NETWORK_DATA_WIDTH, type=int,
                        help="Bitwidth of network address bus")
    parser.add_argument("--host-coord-x", default=HOST_INTERFACE_COORD_X, type=int,
                        help="X coordinate of the host interface")
    parser.add_argument("--host-coord-y", default=HOST_INTERFACE_COORD_Y, type=int,
                        help="Y coordinate of the host interface")
    parser.add_argument("--network-x", default=NETWORK_DIMENSION_X, type=int,
                        help="X dimension of the network")
    parser.add_argument("--network-y", default=NETWORK_DIMENSION_Y, type=int,
                        help="X dimension of the network")
    parser.add_argument("--mc-version", default=MC_VERSION_ID, type=lambda x: int(x,16),
                        help="MC version")
    parser.add_argument("--desc-ptr", default=DESCRIPTION_PTR, type=lambda x: int(x,16),
                        help="Description Pointer")
    parser.add_argument("--comp-date", default=COMPILATION_DATE, type=lambda x: int(x,16),
                        help="Compilation date")
    parser.add_argument("--vcache-associativity", default=VCACHE_ASSOCIATIVITY, type=int,
                        help="Associativity of the victim cache")
    parser.add_argument("--vcache-sets", default=VCACHE_SETS, type=int,
                        help="Number of sets in the victim cache")
    parser.add_argument("--vcache-block-size-in-words", default=VCACHE_BLOCK_SIZE_IN_WORDS, type=int,
                        help="Block size in words of the victim cache")
    parser.add_argument("--vcache-stripe-size-in-words", default=VCACHE_STRIPE_SIZE_IN_WORDS, type=int,
                        help="Stripe size in words of the victim cache")
    parser.add_argument("release", nargs=3, action=ReleaseRepoAction,
                        help='Repositories that this release is based on as repo_name@commit_id')

    args = parser.parse_args()
    return args

def print_item(item):
  print("{:032b}".format(item))

def main():
    args = parse_args()
    print_item(args.mc_version)
    print_item(args.comp_date)
    print_item(args.network_addr_width)
    print_item(args.network_data_width)
    print_item(args.network_x)
    print_item(args.network_y)
    print_item(args.host_coord_x)
    print_item(args.host_coord_y)
    print_item(args.desc_ptr)
    print_item(args.bsg_ip_cores)
    print_item(args.bsg_manycore)
    print_item(args.bsg_f1)
    print_item(args.vcache_associativity)
    print_item(args.vcache_sets)
    print_item(args.vcache_block_size_in_words)
    print_item(args.vcache_stripe_size_in_words)

if __name__ == "__main__":
    main()
