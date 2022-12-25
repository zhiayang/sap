#!/usr/bin/env python3
"""
Usage:
    ./sort_includes.py
        Takes source file in stdin and outputs sorted source file to stdout
    ./sort_includes.py -i <file>
        Sorts the file in place
"""

import collections

def folder(line):
    components = line.split('"')
    if len(components) < 3:
        return ""
    else:
        folder = components[1].split('/')
        return '/'.join(folder[:-1])

def sort_includes(source):
    out=''
    includes=[]
    for line in source.split('\n'):
        if line.startswith('#include'):
            includes.append(line)
        else:
            includes_by_dir = collections.defaultdict(list)
            for i in includes:
                includes_by_dir[folder(i)].append(i)
            for includes_in_dir in includes_by_dir.values():
                includes_in_dir.sort(key=lambda line: (len(line[:line.find('//')].rstrip()), line))
            includes2 = list(includes_by_dir.items())
            includes2.sort(key=lambda pair: (len(pair[0]), pair[0]))

            for num, includes_in_dir in enumerate(includes2):
                for i in includes_in_dir[1]:
                    out += i
                    out += '\n'
                if num != len(includes2) - 1:
                    out += '\n'
            includes.clear()
            out += line
            out += '\n'
    return out[:-1]

if __name__ == '__main__':
    import sys
    if len(sys.argv) == 1:
        print(sort_includes(sys.stdin.read()), end='')
    elif len(sys.argv) == 3 and sys.argv[1] == '-i':
        f = open(sys.argv[2])
        sorted_file = sort_includes(f.read());
        f.close()
        f = open(sys.argv[2], 'w')
        f.write(sorted_file)
    else:
        print(__doc__.strip())
