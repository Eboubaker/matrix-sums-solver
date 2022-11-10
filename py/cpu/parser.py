import os
import re
from sys import argv, stdin

import numpy as np
import select

TAG_ARGS=2

from mpi4py import MPI
comm = MPI.COMM_WORLD
size = comm.Get_size()
rank = comm.Get_rank()


def arg_parse_panic(msg):
    """"
    exit and kill the mpi environment
    """
    print(msg)
    print("Usage:")
    print("solver.py row_sums col_sums <hash>")
    print("where row_sums col_sums are array of the format [x, y, z, ...n]")
    print("hash is optional, if not provided all possibilities that match the provided sums will be printed")
    print("if hash is provided only 1 matrix will be printed that matches the hash")
    if rank == 0 and size > 1:
        send = {'argv': msg}
        for i in range(1, size):
            comm.send(send, dest=i, tag=TAG_ARGS)
    exit(1)


def parse_sums(sums_as_str):
    match = re.fullmatch(r"\[(\d(,\d)*?)]", sums_as_str)
    if match:
        return np.fromstring(match.groups()[0], dtype=int, sep=',')
    return None


def read_args(args):
    """
    read arguments from command line or from pipe
    """
    if os.name == 'nt':  # windows
        read_pipe = len(args) < 3 and not stdin.isatty()  # when args are piped stdin will not be tty device
    else:
        read_pipe = select.select([stdin, ], [], [], 0.0)[0]
    if read_pipe:
        args = [args[0]]
        args.extend(stdin.readline().split(' '))  # read from pipe
    return args


# argv = ["C:/", "[2,1,1]", "[0,2,2]", "cbf0b24d94055b7cfd5df42cfb1d95bb"]
if size <= 1:
    argv = read_args(argv)
else:
    # also handle args if they are piped
    if rank == 0:
        argv = read_args(argv)
    else:
        # rank 0 will send args after validation
        data = comm.recv(source=0, tag=TAG_ARGS)
        if isinstance(data['argv'], str):
            exit(0)  # rank 0 showed the message we just exit
        else:
            argv = data['argv']

rows_sums = parse_sums(argv[1])
col_sums = parse_sums(argv[2].rstrip('\n'))
matrix_hash = argv[3].rstrip('\n') if len(argv) > 3 else None
seed = argv[4].rstrip('\n') if len(argv) > 4 else None

if rank == 0:
    if len(argv) < 3:
        arg_parse_panic("error: invalid number of arguments")
    if rows_sums is None:
        arg_parse_panic("error: cant parse first argument {} as array, array format should be [x,x,x]".format(argv[1]))
    if col_sums is None:
        arg_parse_panic("error: cant parse second argument {} as array, array format should be [x,x,x]".format(argv[2]))
    if matrix_hash is not None and len(matrix_hash) < 0:
        arg_parse_panic(
            "error: third argument matrix must not be empty"
        )
    if matrix_hash is not None and not re.findall(r"([a-fA-F\d]{32})", matrix_hash):
        arg_parse_panic(
            "error: third argument matrix hash should be a hex hash [A-F0-9] got a character that is outside this range"
        )
    if rows_sums.sum() != col_sums.sum():
        arg_parse_panic(
            "error: array1 sum({}) does not match array2 sum({}) this problem cannot be solved".format(
                rows_sums.sum(), col_sums.sum())
        )
if rank == 0 and size > 1:
    send = {'argv': argv}
    for i in range(1, size):
        comm.isend(send, dest=i, tag=TAG_ARGS)
