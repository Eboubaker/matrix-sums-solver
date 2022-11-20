import random
import threading
import time
from timeit import default_timer as timer

import numpy as np
from mpi4py import MPI

from lib import m_hash_digest

comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()
start = timer()
other_ranks = [r for r in range(size) if r != rank]

TAG_EXIT = 1
TAG_TASK = 2
stop = threading.Event()

def random_rank():
    return random.choice(other_ranks)


def solve_recursive(nb_rows: int, nb_cols: int, matrix: np.ndarray, srow: int, scol: int, row_sums: np.ndarray, col_sums: np.ndarray, hashdigest: bytes):
    # print(f"{pid} recurse")
    for row in range(srow, nb_rows):
        for col in range(scol, nb_cols):
            if row_sums[row] == 0 or col_sums[col] == 0 or (
                    # if next columns with non-zero sum in this cell's row is greater than remaining row sum
                    # and next rows with non-zero sum in this cell's column is greater than remaining column sum
                    # then we are allowed to put zero in this cell.
                    np.count_nonzero(col_sums[col:]) > row_sums[row]
                    and np.count_nonzero(row_sums[row:]) > col_sums[col]
            ):
                # we can put zero in this cell [row,col]
                # so we can leave it empty(default is zero)
                if col_sums[col] != 0 and row_sums[row] != 0:  # there is still remaining sums
                    # we also can add 1 in this cell [row, col]
                    # we will set this cell to 1 in a new copy and send the task to another rank,
                    # another rank will continue generating and solving the next cells
                    next_matrix = np.copy(matrix)
                    next_row_sums = np.copy(row_sums)
                    next_col_sums = np.copy(col_sums)
                    next_matrix[row, col] = 1
                    next_row_sums[row] -= 1
                    next_col_sums[col] -= 1
                    next_col = col + 1
                    next_row = row
                    if next_col >= nb_cols:
                        # then start from next row
                        next_col = 0
                        next_row += 1
                    d = random_rank()
                    # print(f"{rank} send to {d}")
                    comm.send((next_matrix, next_row, next_col, next_row_sums, next_col_sums), dest=d, tag=TAG_TASK)
            else:
                # this cell must be set to 1, according to the sums there is no other posibility
                # set it to 1 and reduce 1 from row_sums and 1 from col_sums of this cell's row and column
                matrix[row, col] = 1
                row_sums[row] -= 1
                col_sums[col] -= 1
        scol = 0  # reset starting column of first iteration

    # print(pid, "checking", matrix)
    if m_hash_digest(matrix) == hashdigest:
        return matrix


def parallel_solve(rows: int, cols: int, hashdigest: bytes):
    global start
    while True:
        # print(f"{rank} wait task tag {TAG_TASK}", flush=True)
        next_matrix, next_row, next_col, next_row_sums, next_col_sums = comm.recv(tag=TAG_TASK)
        # print(f"{rank} got new task")
        matrix = solve_recursive(rows, cols, next_matrix, next_row, next_col, next_row_sums, next_col_sums, hashdigest)
        if matrix is not None:
            print(f"rank {rank} solved after {timer() - start}s")
            print(matrix, flush=True)
            for r in other_ranks:
                comm.send(True, dest=r, tag=TAG_EXIT)
            stop.set()
            return

def worker_thread(rows, cols, hexdigest):
    global start
    start = timer()
    parallel_solve(rows, cols, hexdigest)


def main():
    from parser import rows_sums, col_sums, matrix_hash, seed
    if rank == 0:
        print(f"solving for row_sums({rows_sums}) col_sums({col_sums}) hash({matrix_hash}) seed({seed})", flush=True)
        d = random_rank()
        # print(f"{rank} send first to {d} tag {TAG_TASK}")
        comm.send((
            np.zeros(shape=(len(rows_sums), len(col_sums)), dtype=int),  # initial matrix
            0,  # start from row 0
            0,  # and column 0
            rows_sums,
            col_sums
        ), dest=d, tag=TAG_TASK)
    t = threading.Thread(target=worker_thread, args=(len(rows_sums), len(col_sums), bytes.fromhex(matrix_hash)))
    t.daemon = True
    t.start()
    req = comm.irecv(tag=TAG_EXIT)
    while not req.Test() and not stop.is_set():
        time.sleep(.2)
        pass

main()
# if rank == 0:
#     comm.send((
#         np.zeros(shape=(3, 3), dtype=int),  # initial matrix
#         0,  # start from row 0
#         0,  # and column 0
#         np.array([1, 2]),
#         np.array([1, 2])
#     ), dest=1, tag=TAG_TASK)
# else:
#     def r():
#         print(comm.recv(tag=TAG_TASK))
#     threading.Thread(target=r).start()
#     # print("program done", flush=True)
