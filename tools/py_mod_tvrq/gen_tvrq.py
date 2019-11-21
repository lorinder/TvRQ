#!/usr/bin/env python

"""
Tool to create a _tvrq python module.

The _tvrq module gives access to the the RQ API and the internal calls
of the implementation.  
"""

import getopt
import glob
import os
import re
import sys

from cffi import FFI

#
#  Functionality to extract the C headers
#

def read_header(fname, pp_block_set, trace=False):
    """Minimally process a header file and return as a string."""
    # pp are preprocessor tracking tokens
    # - pp_stack is the state of stack, each entry a list [token, # blocked_flag]
    # - pp_block_count is the number of blocked states in the stack.
    pp_stack = []
    pp_block_count = 0

    out = ""
    with open(fname, 'r') as fp:
        l = ""
        for lr in fp:
            skip_this_line = False

            # Concatenate lines ending with \ with next one.
            lr = lr.strip()
            if len(lr) > 0 and lr[-1] == '\\':
                lr = lr[:-1]
                l += lr
                continue
            l += lr

            # Check preproc
            if re.match("#if", l):
                skip_this_line = True
                if l in pp_block_set:
                    pp_block_count += 1
                    pp_stack.append([l, 1])
                else:
                    pp_stack.append([l, 0])
                if trace:
                    out += "##pp_block_set %s\n" % (pp_block_set)
                    out += "##if %s\n" % pp_stack[-1]
            elif re.match("#(elif|else)", l):
                skip_this_line = True
                assert len(pp_stack) > 0
                pp_block_count -= pp_stack[-1][1]
                pp_stack[-1][0] += "$$$" + l
                pp_stack[-1][1] = int(pp_stack[-1][0] in pp_block_set)
                pp_block_count += pp_stack[-1][1]
                if trace:
                    out += "##else %s\n" % pp_stack[-1]
            elif re.match("#endif", l):
                skip_this_line = True
                assert len(pp_stack) > 0
                pp_block_count -= pp_stack[-1][1]
                pp_stack = pp_stack[:-1]
                if trace:
                    out += "##endif %s\n" % \
                              ("" if len(pp_stack)==0 else pp_stack[-1])
            elif re.match('#include', l):
                skip_this_line = True
            elif re.match('#define', l):
                skip_this_line = True
            elif re.match('#undef', l):
                skip_this_line = True
            elif re.match('RQAPI', l):
                skip_this_line = True
            if pp_block_count == 0 and not skip_this_line:
                out += l + '\n'
            l = ""
    return out

headers = [
        ('gf256.h',         set()),
        ('m256v.h',         {'#ifndef PY_CFFI'}),
        ('m2v.h',           {'#ifndef PY_CFFI'}),
        ('parameters.h',    set()),
        ('rand.h',          set()),
        ('hdpc.h',          set()),
        ('ldpc.h',          set()),
        ('lt.h',            set()),
        ('rq_matrix.h',     set()),
        ('tuple.h',         set()),
        ('rq_api.h',        {'#ifdef __cplusplus'}),
        ]

def get_header_path(reldir, hnam):
    fl = glob.glob("%s/*/%s" % (reldir, hnam))
    if len(fl) == 0:
        sys.stderr.write("Error:  Header file `%s' not found.\n" % hnam)
        sys.exit(1)
    elif len(fl) > 1:
        sys.stderr.write("Error:  Header file `%s' ambiguous. Matches: %s\n" \
            % (hnam, fl))
        sys.exit(1)
    return fl[0]

def get_all_dependencies(reldir):
    deps = []
    for hnam, _ in headers:
        deps.append(get_header_path(reldir, hnam))
    return deps

def read_all_headers(reldir, trace = False):
    out = ""
    for hnam, pp_block_set in headers:
        out += read_header(
                        get_header_path(reldir, hnam),
                        pp_block_set,
                        trace)
    return out

def get_include_directives():
    out = ""
    for hnam, _ in headers:
        out += ("#include \"%s\"\n" % hnam)
    return out

#
#  The main generator tool
#

(   ACT_GEN_MOD,
    ACT_DEP,
    ACT_HELP,
) = list(range(3))

def main():

    # Read command line args
    action = ACT_GEN_MOD
    reldir = "../../../"
    opts, args = getopt.getopt(sys.argv[1:], "hdx:")
    for o, a in opts:
        if o == '-h':
            action = ACT_HELP
        elif o == '-d':
            action = ACT_DEP
        elif o == '-x':
            reldir = a

    # Run
    if action == ACT_GEN_MOD:
        ffibuilder = FFI()
        ffibuilder.cdef(read_all_headers(reldir))
        ffibuilder.set_source("_tvrq",
            get_include_directives(),
            libraries=[
                'algebra',
                'rfc6330_alg',
                'tvrq',
                'tvrqapi',
                ],
            extra_link_args=[
                '-L../../algebra/',
                '-L../../rfc6330_alg/',
                '-L../../tvrq/',
                '-L../../api/',
                '-Wl,-rpath=%s/../../api/' % (os.getcwd(),),
                ],
            include_dirs=[reldir + 'tvrq',
                    reldir + 'algebra',
                    reldir + 'rfc6330_alg',
                    reldir + 'api'])
        ffibuilder.compile(verbose=False)
    elif action == ACT_DEP:
        deps = get_all_dependencies(reldir)
        print(";".join(deps))
    elif action == ACT_HELP:
        print("gen_tvrq.py --- Generates the python _tvrq module.\n"
                "\n"
                "actions:\n"
                "  default is to generate the module\n"
                "  -d      print header file dependencies of module\n"
                "  -h      display help screen and exit\n"
                "\n"
                "flags:\n"
                "  -x <p>  set the relative path; should point to\n"
                "          root of src tree.\n"
        )

if __name__ == "__main__":
    main()
