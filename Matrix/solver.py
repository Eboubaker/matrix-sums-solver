from mpi4py import MPI

from lib_gpu import m_hash

comm = MPI.COMM_WORLD
size = comm.Get_size()
rank = comm.Get_rank()


def _solve(matrix, open_cells, row_sums, col_sums, hash):
    rows, cols = matrix.shape
    while True:
        old_open_sum = open_cells.sum()
        for row in range(0, rows):
            sum = row_sums[row] - matrix[row].sum()
            open = open_cells[row].sum()
            if sum == 0 and open == 0:
                continue  # row was solved before
            if sum == open:  # all open cells in this row must be set to 1 to match this criteria
                for col in range(0, cols):
                    if open_cells[row, col]:  # only set empty cells in the row
                        matrix[row, col] = 1
                        open_cells[row, col] = 0
            elif sum == 0:  # all open cells in this row must be set to 0 to match this criteria
                for col in range(0, cols):
                    if open_cells[row, col]:  # only set empty cells in the row
                        matrix[row, col] = 0
                        open_cells[row, col] = 0

        for col in range(0, cols):
            sum = col_sums[col] - matrix[:, col].sum()
            open = open_cells[:, col].sum()
            if sum == 0 and open == 0:
                continue  # column was solved before
            if sum == open:  # all open cells in this column must be set to 1 to match this criteria
                for row in range(0, rows):
                    if open_cells[row, col]:  # only set empty cells in the column
                        matrix[row, col] = 1
                        open_cells[row, col] = 0
            elif sum == 0:  # all open cells in this column must be set to 0 to match this criteria
                for row in range(0, rows):
                    if open_cells[row, col]:  # only set empty cells in the column
                        matrix[row, col] = 0
                        open_cells[row, col] = 0
        opened = open_cells.sum()
        if opened == 0:
            if hash is not None and m_hash(matrix) == hash:
                # this is the only possibility for the hash
                yield matrix
            else:
                # this is a possible solution
                yield matrix
        elif old_open_sum == opened:
            # matrix next state has many possibilities we need to test if one of them matches the hash
            bits = np.arange(0, opened)
            bits.fill(0)
            for i in range(0, 2 ** opened):
                mrow = 0
                mcol = 0
                next_matrix = matrix.copy()
                next_open_cells = open_cells.copy()
                invalid = False
                for bitIndex in range(0, opened):
                    while open_cells[mrow, mcol] == 0:
                        mcol += 1
                        if mcol == cols:
                            mcol = 0
                            mrow += 1
                            if mrow == rows:
                                raise RuntimeError("unexpected mrow == rows")
                    next_matrix[mrow, mcol] = bits[bitIndex]
                    next_open_cells[mrow, mcol] = 0
                    if next_matrix[mrow].sum() > row_sums[mrow] or next_matrix[:, mcol].sum() > col_sums[mcol]:
                        invalid = True
                        break
                    else:
                        for m in _solve(next_matrix.copy(), next_open_cells.copy(), row_sums, col_sums, hash):
                            yield m
                if invalid:
                    continue
                if i < 2 ** opened - 1:
                    index = 0
                    bits[index] += 1
                    while bits[index] == 2:  # do bit overflow
                        bits[index] = 0
                        index += 1
                        bits[index] += 1

def solve(row_sums, col_sums, hash):
    """"
        if hash is set will return the matrix that solves the problem and matches the hash or None if not found,
        if hash is not provided will print all possible solutions that match rows_sums and col_sums
    """
    if len(row_sums) == 0 or len(col_sums) == 1:
        return None
    matrix = np.matrix(np.ndarray((len(row_sums), len(col_sums)), dtype=int))
    matrix.fill(0)
    open_cells = matrix.copy()
    open_cells.fill(1)
    for matrix in _solve(matrix, open_cells, row_sums, col_sums, hash):
        print(matrix)


if __name__ == "__main__":
    from solver_parser import *

    print("solving for row_sums({}) col_sums({}) hash({}) seed({})".format(rows_sums, col_sums, matrix_hash, seed))
    x = solve(rows_sums, col_sums, matrix_hash)
    if matrix_hash is not None:
        print(x)
