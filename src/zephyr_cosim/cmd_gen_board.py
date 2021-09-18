'''
Created on Sep 18, 2021

@author: mballance
'''
import os
import shutil
from zephyr_cosim import get_share
from vte.template_info import TemplateInfo
from vte.template_engine import TemplateEngine


def gen_board(args):
    
    board_template_dir = os.path.join(get_share(), "board")
    
    if args.o is None:
        args.o = os.path.join(
            os.getcwd(),
            "zephyr",
            "boards",
            "posix",
            args.name)

    if os.path.isdir(args.o):
        if not args.force:
            raise Exception("Output directory %s exists" % args.o)
        else:
            print("Note: removing existing output directory %s" % args.o)
            shutil.rmtree(args.o)

    if not os.path.isdir(args.o):
        os.makedirs(args.o)
        
    # Load up template information
    template = TemplateInfo.mk("zephyr.cosim.board", 
                               os.path.join(board_template_dir, ".vte"))
    TemplateEngine(template, args.o).generate(args.name, {})
    
        
    pass