from mpi4py import MPI

from lib_gpu import m_hash

comm = MPI.COMM_WORLD
size = comm.Get_size()
rank = comm.Get_rank()


def mgenerate(rows, cols):
    bits = np.arange(0, rows * cols)
    bits.fill(0)
    for i in range(0, 2 ** (rows * cols)):
        m = np.flip(np.rot90(np.reshape(bits, (cols, rows))))
        print(bits)
        yield m
        if i < 2 ** (rows * cols) - 1:
            index = 0
            bits[index] += 1
            while bits[index] == 2:
                bits[index] = 0
                index += 1
                bits[index] += 1


def mfilter(matrices, row_sums, col_sums):
    for matrix in matrices:
        invalid = False
        for row in range(0, len(row_sums)):
            if matrix[row].sum() != row_sums[row]:
                invalid = True
        if not invalid:
            for col in range(0, len(col_sums)):
                if np.flip(np.rot90(matrix))[col].sum() != col_sums[col]:
                    invalid = True
        if not invalid:
            yield matrix


def msolve(matrices, checksum):
    for matrix in matrices:
        if m_hash(np.matrix(matrix)) == checksum:
            return matrix
    return None


from solver_parser import *

print("solving for row_sums({}) col_sums({}) hash({}) seed({})".format(rows_sums, col_sums, matrix_hash, seed))

print(
    msolve(mfilter(mgenerate(len(rows_sums), len(col_sums)), rows_sums, col_sums), matrix_hash)
)
