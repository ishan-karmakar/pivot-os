#! /usr/bin/env python3

from sys import argv
import os

OBJCOPY = os.environ['objcopy']

in_path, out_path = argv[1:]
out_path = f"{os.getcwd()}/{out_path}"
os.chdir(os.path.dirname(in_path))
os.system(f"{OBJCOPY} -O elf64-x86-64 -B i386 -I binary {os.path.basename(in_path)} {out_path}")