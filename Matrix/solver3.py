from math import log
from threading import Thread

import numpy as np
from mpi4py import MPI

from conf import TAG_DONE
from lib import m_hash

comm = MPI.COMM_WORLD
size = comm.Get_size()
rank = comm.Get_rank()

stop = [False]


def m_generate(rows, cols, parts_count=None, part=None):
    bits_len = rows * cols
    bits = np.arange(0, bits_len)
    bits.fill(0)
    end_bit = bits_len  # last bit
    if part is not None and parts_count:
        mask_len = int(log(parts_count, 2))
        end_bit = bits_len - mask_len
        shift = 0
        for index in range(end_bit, bits_len):
            if (part >> shift) & 1:
                bits[index] = 1
            shift += 1
    possibilities_count = 2 ** end_bit
    # print(rank, bits_len, end_bit, possibilities_count)
    for i in range(possibilities_count):
        #print(rank, bits)
        if stop[0]:
            return
        # TODO: instead of flip(rot90()) calculate the whole thing in reverse, flip and rot90 might be expensive to do
        m = np.flip(np.rot90(np.reshape(bits, (cols, rows))))
        yield m
        if i < possibilities_count - 1:
            index = 0
            bits[index] += 1
            while bits[index] == 2:
                bits[index] = 0
                index += 1
                bits[index] += 1


def m_filter(row_sums, col_sums, matrices):
    for matrix in matrices:
        invalid = False
        for row in range(0, len(row_sums)):
            if matrix[row].sum() != row_sums[row]:
                invalid = True
        if not invalid:
            for col in range(0, len(col_sums)):
                if matrix[:, col].sum() != col_sums[col]:
                    invalid = True
        if not invalid:
            yield matrix


def m_solve(checksum, matrices):
    for matrix in matrices:
        if m_hash(np.matrix(matrix)) == checksum:
            return matrix
    return None


def main():
    from solver_parser import rows_sums, col_sums, matrix_hash, seed
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

    def listen():
        handle = comm.irecv(tag=TAG_DONE)
        while not stop[0]:
            if handle.Test():
                #print(rank, "received stop signal")
                stop[0] = True

    t = Thread(target=listen)
    t.daemon = True
    t.start()

    result = m_solve(
        matrix_hash,
        m_filter(
            rows_sums,
            col_sums,
            m_generate(
                len(rows_sums),
                len(col_sums),
                size,
                rank
            )
        )
    )
    if result is not None:
        print(rank, result)
        for i in range(size):
            if i == rank:
                continue
            #print(rank, "send exit to", i)
            comm.send(obj=True, dest=i, tag=TAG_DONE)
    stop[0] = True

main()
#print(rank, "program done", flush=True)
