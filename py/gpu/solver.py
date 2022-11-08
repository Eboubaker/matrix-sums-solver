import threading
import time
from math import log

import numpy as np
from mpi4py import MPI
from numba import jit
from timeit import default_timer as timer

from conf import TAG_DONE
from lib import m_hash

comm = MPI.COMM_WORLD
size = comm.Get_size()
rank = comm.Get_rank()

stop = False


@jit(target_backend='cuda')
def m_solve(bits, possibilities_count, rows, cols, row_sums, col_sums, matrix_hash):
    global stop
    # print(rank, bits_len, end_bit, possibilities_count)
    for i in range(possibilities_count):
        if stop:
            return
        # print(rank, bits)
        matrix = np.reshape(bits, (rows, cols))
        invalid = False
        for row in range(0, len(row_sums)):
            if matrix[row].sum() != row_sums[row]:
                invalid = True
                break
        if not invalid:
            for col in range(0, len(col_sums)):
                if matrix[:, col].sum() != col_sums[col]:
                    invalid = True
                    break

        if not invalid and matrix_hash == m_hash(matrix):
            return matrix
        # print(m_hash(matrix), bits)
        if i < possibilities_count - 1:
            index = 0
            bits[index] += 1
            while bits[index] == 2:
                bits[index] = 0
                index += 1
                bits[index] += 1


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
            global stop
            handle = comm.irecv(tag=TAG_DONE)
            while not stop:
                if handle.Test():
                    # print(rank, "received stop signal")
                    stop = True
                time.sleep(.2)

        t = threading.Thread(target=listen)
        t.daemon = True
        t.start()

    bits_len = len(rows_sums) * len(col_sums)
    bits = np.arange(0, bits_len, dtype=np.float64)
    bits.fill(0)
    end_bit = bits_len  # last bit
    if rank is not None and size:
        mask_len = int(log(size, 2))
        end_bit = bits_len - mask_len
        shift = 0
        for index in range(end_bit, bits_len):
            if (rank >> shift) & 1:
                bits[index] = 1
            shift += 1
    possibilities_count = 2 ** end_bit
    start = timer()
    result = m_solve(
        bits,
        possibilities_count,
        len(rows_sums),
        len(col_sums),
        rows_sums,
        col_sums,
        int(matrix_hash)
    )
    if result is not None:
        print(f"rank {rank} solved in {timer() - start}s")
        print(result)
        if size > 1:
            for i in range(size):
                if i == rank:
                    continue
                # print(rank, "send exit to", i)
                comm.send(obj=True, dest=i, tag=TAG_DONE)
    stop = True


main()
# print(rank, "program done", flush=True)
