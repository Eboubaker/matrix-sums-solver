import multiprocessing
import os
import traceback
from timeit import default_timer as timer

start = timer()
import numpy as np

from lib import m_hash_digest

pid = multiprocessing.process.current_process().pid

work_buffer = 0


def solve_recursive(q: multiprocessing.Queue, matrix: np.ndarray, srow: int, scol: int, row_sums: np.ndarray, col_sums: np.ndarray, static_row_sums: np.ndarray, static_col_sums: np.ndarray, hashdigest):
    global work_buffer
    for row in range(srow, len(row_sums)):
        for col in range(scol, len(col_sums)):
            row_open_cells = np.count_nonzero(col_sums[col:])  # count next columns with non-zero sum in this cell row
            col_open_cells = np.count_nonzero(row_sums[row:])  # count next rows with non-zero sum in this cell column
            if row_sums[row] == 0 or col_sums[col] == 0 or (row_open_cells > row_sums[row] and col_open_cells > col_sums[col]):
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
                    if next_col >= len(col_sums):
                        # then start from next row
                        next_col = 0
                        next_row += 1
                    bottom_line = row == len(row_sums) - 1  # optimization
                    if (work_buffer > 1024 or q.empty()) and not bottom_line:
                        # there are processes without work, or we made a lot of tasks recently, we will put this task in the queue
                        # to be executed by another process
                        work_buffer = 0
                        q.put((next_matrix, next_row, next_col, next_row_sums, next_col_sums))
                    else:
                        work_buffer += 1
                        m = solve_recursive(q, next_matrix, next_row, next_col, next_row_sums, next_col_sums, static_row_sums, static_col_sums, hashdigest)
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


def parallel_solve(indx, queue: multiprocessing.Queue, exit_code: multiprocessing.Queue, rows_sums, col_sums, hashdigest):
    global start
    pstart = timer()
    while True:
        next_matrix, next_row, next_col, next_row_sums, next_col_sums = queue.get()
        # print(f"{pid} got new task")
        try:
            matrix = solve_recursive(queue, next_matrix, next_row, next_col, next_row_sums, next_col_sums, rows_sums, col_sums, hashdigest)
        except:
            print(f"error in process {pid}[{indx}], {traceback.format_exc()}", flush=True)
            exit_code.put(1)
            return
        if matrix is not None:
            print(f"process {pid}[{indx}] solved after {timer() - pstart}s (real time)")
            print(matrix, flush=True)
            exit_code.put(0)
            return


def main():
    from parser import rows_sums, col_sums, matrix_hash, seed
    if rows_sums is None:
        return
    processes_count = os.cpu_count()
    print("solving for row_sums({}) col_sums({}) hash({}) seed({})".format(rows_sums, col_sums, matrix_hash, seed))
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
        processes.append(multiprocessing.Process(target=parallel_solve, args=(i, queue, exit_code, rows_sums, col_sums, bytes.fromhex(matrix_hash))))
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
