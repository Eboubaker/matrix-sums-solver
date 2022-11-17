import numpy as np

from mpi4py import MPI
comm = MPI.COMM_WORLD
size = comm.Get_size()
rank = comm.Get_rank()
print("rank", rank)
def generate_recursive(matrix, srow, scol, row_sums, col_sums, static_row_sums, static_col_sums):
    for row in range(srow, len(row_sums)):
        for col in range(scol, len(col_sums)):
            row_next_ones = np.count_nonzero(col_sums[col:])
            col_next_ones = np.count_nonzero(row_sums[row:])
            if row_sums[row] == 0 or col_sums[col] == 0 or (row_next_ones > row_sums[row] and col_next_ones > col_sums[col]):
                # we can put zero in this cell [row,col]
                # so we can leave it empty
                if col_sums[col] != 0 and row_sums[row] != 0:
                    # we can add 1 in this cell [row, col]
                    # we will set this cell to 1 and recurse call to continue generating the next cells
                    next_matrix = np.copy(matrix)
                    next_row_sums = np.copy(row_sums)
                    next_col_sums = np.copy(col_sums)
                    next_matrix[row, col] = 1
                    next_row_sums[row] -= 1
                    next_col_sums[col] -= 1
                    next_col = col + 1
                    next_row = row
                    if next_col >= len(col_sums):
                        next_col = 0
                        next_row += 1
                    for m in generate_recursive(next_matrix, next_row, next_col, next_row_sums, next_col_sums, static_row_sums, static_col_sums):
                        yield m
            elif col_sums[col] != 0 and row_sums[row] != 0:
                # this cell must be set to 1
                # set it to 1 and reduce 1 from row_sums and 1 from col_sums of this cell
                matrix[row, col] = 1
                row_sums[row] -= 1
                col_sums[col] -= 1
            else:
                assert False, "undefined case(probably invalid sums)"  # should not happen
        scol = 0  # reset starting column

    # check if generated matrix is valid and yield it
    for row in range(len(static_row_sums)):
        if matrix[row, :].sum() != static_row_sums[row]:
            return  # not valid
    for col in range(len(static_col_sums)):
        if matrix[:, col].sum() != static_col_sums[col]:
            return  # not valid
    yield matrix


def generate(row_sums, col_sums):
    """
    only generate matrices that match the sums
    """
    yield from generate_recursive(np.zeros(shape=(len(row_sums), len(col_sums)), dtype=int), 0, 0, np.copy(row_sums), np.copy(col_sums), row_sums, col_sums)


"""
for this matrix sums
2 1 0 1 1
x x x x x 1
x x x x x 2
x x x x x 2
"""
for m in generate(row_sums=np.array([2, 3, 2, 1, 2]), col_sums=np.array([2, 3, 1, 2, 2])):
    print(m)
    print("*****")
