#!/usr/bin/env python3

import os
import sys
import time
import shutil
import filecmp
import subprocess

def header_mtimes_indicate_that_we_need_to_rebuild(header_file):
	if not os.path.isfile(header_file):
		return True

	for inc in open(header_file, "r").readlines():
		a, b = list(map(lambda x: x.strip(), inc.split("//")))
		hdr_name = a.split('"')[1]
		mod_time = float(b.split('=')[1])

		if not os.path.isfile(hdr_name) or os.path.getmtime(hdr_name) != mod_time:
			# print(f"rebuilding {header_file} because {hdr_name} changed")
			return True

	return False

def touch_if_exists(file):
	if os.path.exists(file):
		os.utime(file)

def make_deps(output_name, args):
	compiler_output = subprocess.check_output(args).decode()

	a, b = compiler_output.split(':')
	header_deps = ' '.join(b.splitlines()).replace('\\', '').split()[1:]
	header_deps.sort()

	# generate the .d file, but first remove the .cpp from the dependency list
	with open(f"{output_name}.d", "w") as dep_file:
		dep_file.write(f"{a}:")
		for h in header_deps:
			dep_file.write(f" {h}")
		dep_file.write("\n")

	with open(f"{output_name}.inc.new", "w") as inc_file:
		for h in header_deps:
			if h.endswith(".inc"):
				continue
			mtime = os.path.getmtime(h)
			tmp = f"#include \"{h}\""
			inc_file.write(f"{tmp:<60}// mtime={mtime}\n")



def are_files_different(a, b):
	return not filecmp.cmp(a, b, shallow=False)



def make_pch(output_name, pch_args, pch_obj_args):
	old_inc = f"{output_name}.inc"
	new_inc = f"{output_name}.inc.new"

	# only compile the file if either the file doesn't exist, or the new one is different
	if not os.path.exists(old_inc) or are_files_different(old_inc, new_inc):
		os.replace(new_inc, old_inc)

		# we must delete the pch first
		if os.path.isfile(f"{output_name}.gch"):
			os.remove(f"{output_name}.gch")

		# run the compiler
		print(f"  # {'/'.join(output_name.split('/')[1:])}.gch")
		subprocess.check_output(pch_args)

		# if, after building the pch, we realised that we *again* need to rebuild, then
		# we must delete the pch and let the other person do it.
		if header_mtimes_indicate_that_we_need_to_rebuild(f"{output_name}.inc"):
			os.remove(f"{output_name}.gch")
			return

		# move the gch to the real file. this is atomic, to avoid races.
		os.replace(f"{output_name}.gch.tmp", f"{output_name}.gch")

		# check if the compiler is clang. if so, we should also compile the gch into an object.
		if "clang" in pch_args[0]:
			print(f"  # {'/'.join(output_name.split('/')[1:])}.gch.o")
			subprocess.check_output(pch_obj_args)

			# same deal with a temp file
			os.replace(f"{output_name}.gch.o.tmp", f"{output_name}.gch.o")
		else:
			# we copy the header so that gcc can at least use a precompiled header
			shutil.copy(old_inc, f"{output_name}.h")

	elif not os.path.exists(old_inc):
		os.replace(new_inc, old_inc)


	# if we don't need to generate the pch, we still touch it so that make knows not to call us again
	os.utime(f"{output_name}.gch")
	if os.path.exists(f"{output_name}.gch.o"):
		os.utime(f"{output_name}.gch.o")


def main():
	if len(sys.argv) < 2:
		print("usage: ./generate_pch.py <output_basename> ----make-deps [compiler_args]... ----make-pch [...] ----make-pchobj [...]")
		sys.exit(1)

	output_name = sys.argv[1]

	k1 = sys.argv[2:].index("----make-pch")
	k2 = sys.argv[2:].index("----make-pchobj")

	args_make_deps = sys.argv[2:][1:k1]
	args_make_pch  = sys.argv[2:][k1+1:k2]
	args_make_pch_obj = sys.argv[2:][k2+1:]

	touch_if_exists(f"{output_name}.gch")
	touch_if_exists(f"{output_name}.gch.o")

	if not header_mtimes_indicate_that_we_need_to_rebuild(f"{output_name}.inc"):
		return

	make_deps(output_name, args_make_deps)
	make_pch(output_name, args_make_pch, args_make_pch_obj)





if __name__ == "__main__":
	try:
		main()
	except KeyboardInterrupt:
		pass
