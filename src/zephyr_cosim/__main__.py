'''
Created on Sep 16, 2021

@author: mballance
'''
import argparse
import os

from zephyr_cosim import get_share
from zephyr_cosim.cmd_gen_board import gen_board


def cmd_soc_root(args):
    print("%s" % os.path.join(get_share(), "zephyr"))
    
def cmd_sv_files(args):
    share = get_share()
    
    tblink_rpc_sv = os.path.join(
        os.path.dirname(share),
        "packages/tblink-rpc-hdl/src/hvl/tblink_rpc.sv")
    print("%s" % (tblink_rpc_sv,))

def getparser():
    parser = argparse.ArgumentParser()
    subparser = parser.add_subparsers()
    subparser.required = True
    subparser.dest = 'command'
    
    soc_root_cmd = subparser.add_parser("soc-root",
        help="Returns the path to SOC_ROOT")
    soc_root_cmd.set_defaults(func=cmd_soc_root)
    
    gen_board_cmd = subparser.add_parser("gen-board",
        help="Generates a template Zephyr board definition")
    gen_board_cmd.set_defaults(func=gen_board)
    gen_board_cmd.add_argument("name",
        help="Specifies the board's identifier")
    gen_board_cmd.add_argument("-o",
        help="Specifies the output directory for the board")
    gen_board_cmd.add_argument("-f", "--force",
        dest="force", action="store_true",
        help="Forces overwrite of an existing board directory")
    
    sv_files_cmd = subparser.add_parser("sv-files",
        help="Returns the list of SystemVerilog files to compile")
    sv_files_cmd.set_defaults(func=cmd_sv_files)
    
    
    return parser

def main():
    parser = getparser()
    
    args = parser.parse_args()
    
    args.func(args)


if __name__ == "__main__":
    main()
    