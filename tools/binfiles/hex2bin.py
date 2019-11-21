#!/usr/bin/env python

from __future__ import print_function

import sys

orig = sys.stdin.read()

v = 0
isfirst = True
for x in orig:
    if x.isspace():
        continue
    offs = "0123456789abcdef".find(x.lower())
    if offs == -1:
        sys.stderr.write("Error:  Invalid character `%s'." % x)
        sys.exit(1)

    # Accumulate two chars.
    if isfirst:
        v = offs
        isfirst = False
    else:
        bout = (16*v + offs).to_bytes(1, byteorder='big')
        sys.stdout.buffer.write(bout)
        v = 0
        isfirst = True
