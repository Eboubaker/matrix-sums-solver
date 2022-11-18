import multiprocessing
import os
import traceback
from timeit import default_timer as timer
from typing import Optional

start = timer()
import numpy as np

from lib import m_hash_digest

pid = multiprocessing.process.current_process().pid

work_buffer = 0


def solve_recursive(q: Optional[multiprocessing.Queue], nb_rows: int, nb_cols: int, matrix: np.ndarray, srow: int, scol: int, row_sums: np.ndarray, col_sums: np.ndarray, hashdigest: bytes):
    # print(f"{pid} recurse")
    for row in range(srow, nb_rows):
        for col in range(scol, nb_cols):
            if row_sums[row] == 0 or col_sums[col] == 0 or (
                    np.count_nonzero(col_sums[col:]) > row_sums[row]  # check next columns with non-zero sum in this cell row
                    and np.count_nonzero(row_sums[row:]) > col_sums[col]  # check next rows with non-zero sum in this cell column
            ):
                # we can put zero in this cell [row,col]
                # so we can leave it empty(default is zero)
                if col_sums[col] != 0 and row_sums[row] != 0:
                    # we also can add 1 in this cell [row, col]
                    # we will set this cell to 1 in a new copy and put the task in the queue,
                    # another process will continue generating and solving the next cells
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
                    if q is not None and q.empty():
                        # there are processes without work, or we made a lot of tasks recently, we will put this task in the queue
                        # to be executed by another process
                        q.put((next_matrix, next_row, next_col, next_row_sums, next_col_sums))
                    else:
                        m = solve_recursive(q, nb_rows, nb_cols, next_matrix, next_row, next_col, next_row_sums, next_col_sums, hashdigest)
                        if m is not None:
                            return m
            else:
                # this cell must be set to 1
                # set it to 1 and reduce 1 from row_sums and 1 from col_sums of this cell's [row,col]
                matrix[row, col] = 1
                row_sums[row] -= 1
                col_sums[col] -= 1
        scol = 0  # reset starting column

    # print(pid, "checking", matrix)
    if m_hash_digest(matrix) == hashdigest:
        return matrix


def parallel_solve(indx, queue: multiprocessing.Queue, exit_code: multiprocessing.Queue, rows, cols, hashdigest):
    global start
    pstart = timer()
    while True:
        next_matrix, next_row, next_col, next_row_sums, next_col_sums = queue.get()
        # print(f"{pid} got new task")
        try:
            matrix = solve_recursive(queue, rows, cols, next_matrix, next_row, next_col, next_row_sums, next_col_sums, hashdigest)
        except:
            print(f"error in process {pid}[{indx}], {traceback.format_exc()}", flush=True)
            exit_code.put(1)
            return
        if matrix is not None:
            print(f"process {pid}[{indx}] solved after {timer() - pstart}s")
            print(matrix, flush=True)
            exit_code.put(0)
            return


def main():
    global start
    from parser import rows_sums, col_sums, matrix_hash, seed
    if rows_sums is None:
        return
    processes_count = os.cpu_count()
    print("solving for row_sums({}) col_sums({}) hash({}) seed({})".format(rows_sums, col_sums, matrix_hash, seed))
    if len(rows_sums) * len(col_sums) <= 49:
        print(f"using main process(small matrix)")
        # creating and managing processes will be more time consuming than to solve this small matrix with 1 process
        start = timer()
        m = solve_recursive(
            None,
            len(rows_sums),
            len(col_sums),
            np.zeros(shape=(len(rows_sums), len(col_sums)), dtype=int),  # initial matrix
            0,  # start from row 0
            0,  # and column 0
            np.copy(rows_sums),
            np.copy(col_sums),
            bytes.fromhex(matrix_hash)
        )
        if m is not None:
            print(f"process {pid}[main] solved after {timer() - start}s")
            print(m, flush=True)
        else:
            print("not solved, maybe invalid hash?")
        exit(0)
    print(f"using {processes_count} processes")
    queue = multiprocessing.Queue()  # stores running process ids to be killed when a process finds the solution
    exit_code = multiprocessing.Queue()  # to send exit code to main process
    queue.put((
        np.zeros(shape=(len(rows_sums), len(col_sums)), dtype=int),  # initial matrix
        0,  # start from row 0
        0,  # and column 0
        np.copy(rows_sums),
        np.copy(col_sums),
    ))
    processes = []
    for i in range(processes_count):
        processes.append(multiprocessing.Process(target=parallel_solve, args=(i, queue, exit_code, len(rows_sums), len(col_sums), bytes.fromhex(matrix_hash))))
        processes[i].start()
    if 0 != exit_code.get():
        print("not solved, maybe invalid hash?")
    for i in range(len(processes)):
        try:
            processes[i].kill()
        except:
            pass
    print(f"controller process ended after {timer() - start}s (includes process spawning)")


# print(f"process {pid} started")
if __name__ == '__main__':
    main()
    # print("program done", flush=True)
