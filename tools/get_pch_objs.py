#!/usr/bin/env python3

import os
import sys

if len(sys.argv) < 3:
	print("?????")
	sys.exit(1)

if "clang" not in sys.argv[1]:
	sys.exit(0)

for cpp_file in sys.argv[2:]:
	if not os.path.exists(f"{cpp_file}.gch") or not os.path.exists(f"{cpp_file}.gch.o"):
		continue

	obj_mtime = os.path.getmtime(f"{cpp_file}.o")
	pch_mtime = os.path.getmtime(f"{cpp_file}.gch")
	if pch_mtime <= obj_mtime:
		print(f" {cpp_file}.gch.o", end='')

sys.stdout.flush()
