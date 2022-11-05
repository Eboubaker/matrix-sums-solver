import hashlib
import threading
import time
from math import log
from timeit import default_timer as timer

import numpy as np
from mpi4py import MPI

from conf import TAG_DONE

comm = MPI.COMM_WORLD
size = comm.Get_size()
rank = comm.Get_rank()
stop = threading.Event()


def solve(row_sums, col_sums, checksum, parts_count=None, part=None):
    rows = len(row_sums)
    cols = len(col_sums)
    bits_len = rows * cols
    bits = np.arange(0, bits_len)
    bits.fill(0)
    end_bit = bits_len  # last bit
    if part is not None and parts_count:
        assert part < parts_count, "part index not less than parts count"
        mask_len = int(log(parts_count, 2))
        end_bit = bits_len - mask_len
        shift = 0
        for index in range(end_bit, bits_len):
            if (part >> shift) & 1:
                bits[index] = 1
            shift += 1
    pc = 2 ** end_bit  # possibilities_count(2 states for each bit 0 or 1 with 'end_bit' cells)
    # print(rank, bits_len, end_bit, pc)
    for i in range(pc):
        # print(rank, bits)
        matrix = np.reshape(bits, (rows, cols))
        invalid = False
        for row in range(rows):
            if matrix[row].sum() != row_sums[row]:
                invalid = True
                break
        if not invalid:
            for col in range(cols):
                if matrix[:, col].sum() != col_sums[col]:
                    invalid = True
                    break
        if not invalid and hashlib.md5(matrix).hexdigest() == checksum:
            return matrix
        if i < pc - 1:
            index = 0
            bits[index] += 1
            while bits[index] == 2:
                bits[index] = 0
                index += 1
                bits[index] += 1
    return None


def main():
    from parser import rows_sums, col_sums, matrix_hash, seed
    if rank == 0:
        print("solving for row_sums({}) col_sums({}) hash({}) seed({})".format(rows_sums, col_sums, matrix_hash, seed))
    if log(size, 2) != int(log(size, 2)):
        print("number of processors must be a number from power of 2", flush=True)
        comm.Abort(-1)
    bits = len(rows_sums) * len(col_sums)
    if size > bits:
        print(f"number of processors {size} must not be greater than number of bits {bits}, consider increasing "
              f"matrix size or reduce number of processors", flush=True)
        comm.Abort(-1)

    if size > 1:
        def listen():
            handle = comm.irecv(tag=TAG_DONE)
            while not stop.is_set():
                if handle.Test():
                    # print(rank, "received stop signal")
                    stop.set()
                time.sleep(.2)
        t = threading.Thread(target=listen)
        t.daemon = True
        t.start()

    start = timer()
    result = solve(
        rows_sums,
        col_sums,
        matrix_hash,
        size,
        rank,
    )
    if result is not None:
        print(f"rank {rank} solved in {timer() - start}s")
        print(result)
        for i in range(size):
            if i == rank:
                continue
            # print(rank, "send exit to", i)
            comm.send(obj=True, dest=i, tag=TAG_DONE)


main()
# print(rank, "program done", flush=True)
