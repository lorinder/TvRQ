#!/usr/bin/env python

from __future__ import print_function

import sys
import getopt

CHAR_BIT = 8

def print_matrix_header(T):
    write = sys.stdout.write
    write("      ")
    for i in range(T * CHAR_BIT):
        write("|" if i % 10 == 0else " ")
    write(' ')
    for i in range(T * 2):
        write("|" if i % 10 == 0 else " ")
    write('\n')

def print_matrix_row(row_index, row_data, T):
    write = sys.stdout.write
    write("[%3d] " % (row_index))
    for x in row_data:
        for i in range(CHAR_BIT - 1, -1, -1):
            write('1' if ((1 << i) & x) else '0')
    n_missing = len(row_data) - T
    for i in range(n_missing):
        write(CHAR_BIT * '.')
    sys.stdout.write(' ')
    for x in row_data:
        write("%02x" % x)
    for i in range(n_missing):
        write('..')
    write('\n')

def print_matrix(data, T):
    print_matrix_header(T)
    for i, offs in enumerate(range(0, len(data), T)):
        print_matrix_row(i, data[offs:offs+T], T)

def usage():
    print(  "print_symfile.py  --- display binary symbol files.\n"
            "\n"
            "usage: print_symfile.py [-h] [-T #] file...\n"
            "\n"
            "  -h    help and exit\n"
            "\n"
            "  -T #  set symbol size (in bytes)\n"
        )

def main():
    # Default values
    T = 16

    # Parse options
    opts, args = getopt.getopt(sys.argv[1:], "hT:")
    for o, a in opts:
        if o == '-h':
            usage()
            sys.exit(0)
        elif o == '-T':
            T = int(a)

    # Display each argument as matrix
    if len(args) == 0:
        sys.stderr.write("Info:  No file to display given.\n")
    for fn in args:
        with open(fn, 'rb') as fp:
            data = fp.read()
            print_matrix(data, T)


if __name__ == "__main__":
    main()
