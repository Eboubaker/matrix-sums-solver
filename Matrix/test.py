# to measure exec time
from math import log
from timeit import default_timer as timer

import numpy as np
from numba import jit


# normal function to run on cpu
def func(a):
    for i in range(10000000):
        a[i] += 1

    # function optimized to run on gpu


@jit(target_backend='cuda')
def func2(a):
    for i in range(10000000):
        a[i] += 1

@jit(target_backend='cuda')
def m_generate(rows, cols, parts_count=None, part=None):
    bits_len = rows * cols
    bits = np.arange(0, bits_len, dtype=np.float64)
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
        # TODO: instead of flip(rot90()) calculate the whole thing in reverse, flip and rot90 might be expensive to do
        if i < possibilities_count - 1:
            index = 0
            bits[index] += 1
            while bits[index] == 2:
                bits[index] = 0
                index += 1
                bits[index] += 1

def m_generate2(rows, cols, parts_count=None, part=None):
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
        # TODO: instead of flip(rot90()) calculate the whole thing in reverse, flip and rot90 might be expensive to do
        m = np.flip(np.rot90(np.reshape(bits, (cols, rows))))
        if i < possibilities_count - 1:
            index = 0
            bits[index] += 1
            while bits[index] == 2:
                bits[index] = 0
                index += 1
                bits[index] += 1

if __name__ == "__main__":
    n = 10000000
    a = np.ones(n, dtype=np.float64)

    start = timer()
    m_generate2(4, 4)
    print("without GPU:", timer() - start)

    start = timer()
    m_generate(4, 4)
    print("with GPU:", timer() - start)

