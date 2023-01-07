#!/usr/bin/env python3

import os
import sys
import time
import subprocess
import multiprocessing

def main():
	if os.fork() != 0:
		return

	os.setpgrp()
	subprocess.check_output(["make", f"-j{multiprocessing.cpu_count() - 1}", *sys.argv[1:]])


if __name__ == "__main__":
	if len(sys.argv) < 2:
		print("????")
		sys.exit(1)

	os.closerange(2, 10)
	main()
