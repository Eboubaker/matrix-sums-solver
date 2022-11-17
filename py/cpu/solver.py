import queue
import threading
import time
import traceback
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass
from timeit import default_timer as timer

import numpy as np

from lib import m_hash

TAG_ARGS = 2
TAG_DONE = 3
TAG_ADD_WORK = 4
TAG_READY = 5
TAG_BUSY = 6

from mpi4py import MPI

comm = MPI.COMM_WORLD
size = comm.Get_size()
rank = comm.Get_rank()
stop = threading.Event()
workQ = queue.Queue()

state = threading.Lock()
if rank == 0:
    rank_ready = [True for _ in range(size)]
else:
    rank_ready = [False for _ in range(size)]
other_ranks = [i for i in range(size) if i != rank]
sent_tasks = [0 for i in range(size)]

start = 0


@dataclass
class WorkTask:
    matrix: np.ndarray
    srow: int
    scol: int
    row_sums: np.ndarray
    col_sums: np.ndarray


def solve_recursive(matrix, srow, scol, row_sums, col_sums, static_row_sums, static_col_sums, checksum):
    if stop.is_set():
        return
    for row in range(srow, len(row_sums)):
        for col in range(scol, len(col_sums)):
            row_next_ones = np.count_nonzero(col_sums[col:])
            col_next_ones = np.count_nonzero(row_sums[row:])
            if row_sums[row] == 0 or col_sums[col] == 0 or (row_next_ones > row_sums[row] and col_next_ones > col_sums[col]):
                # we can put zero in this cell [row,col]
                # so we can leave it empty
                if col_sums[col] != 0 and row_sums[row] != 0:
                    # we also can add 1 in this cell [row, col]
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
                    sent = False
                    if size > 1:  # check if running in parallel
                        # try to send this task to other non-busy ranks
                        elected = []
                        for r in other_ranks:
                            if rank_ready[r]:
                                elected.append(r)

                        elected.sort(key=lambda r: sent_tasks[r])  # sort by least sent tasks
                        if len(elected) > 0:
                            task = {'next_matrix': next_matrix, 'next_row': next_row, 'next_col': next_col, 'next_row_sums': next_row_sums, 'next_col_sums': next_col_sums}
                            # print(rank, f"sending work task to rank {elected[0]}", flush=True)
                            comm.isend(task, dest=elected[0], tag=TAG_ADD_WORK)
                            sent_tasks[elected[0]] += 1
                            sent = True
                    if not sent:  # task not sent (all ranks are busy), do it yourself
                        yield from solve_recursive(next_matrix, next_row, next_col, next_row_sums, next_col_sums, static_row_sums, static_col_sums, checksum)
            else:
                # this cell must be set to 1
                # set it to 1 and reduce 1 from row_sums and 1 from col_sums of this cell
                matrix[row, col] = 1
                row_sums[row] -= 1
                col_sums[col] -= 1
        scol = 0  # reset starting column
    #
    # # check if generated matrix is valid and yield it
    # for row in range(len(static_row_sums)):
    #     if matrix[row, :].sum() != static_row_sums[row]:
    #         return  # not valid
    # for col in range(len(static_col_sums)):
    #     if matrix[:, col].sum() != static_col_sums[col]:
    #         return  # not valid
    if m_hash(matrix) == checksum:
        yield matrix


def solve(matrix, srow, scol, row_sums, col_sums, static_row_sums, static_col_sums, checksum):
    for result in solve_recursive(matrix, srow, scol, row_sums, col_sums, static_row_sums, static_col_sums, checksum):
        print(f"rank {rank} solved in {timer() - start}s", flush=True)
        print(result)
        for r in other_ranks:
            print(rank, "send exit to", r, flush=True)
            comm.send(obj="Exit", dest=r, tag=TAG_DONE)
        return True


threads = ThreadPoolExecutor(max_workers=1 + (size - 1) * 2)  # exit_listen + (busy + ready) for each other rank


def main():
    global start
    from parser import rows_sums, col_sums, matrix_hash, seed
    if rank == 0:
        print("solving for row_sums({}) col_sums({}) hash({}) seed({})".format(rows_sums, col_sums, matrix_hash, seed))
    # if log(size, 2) != int(log(size, 2)):
    #     print("number of processors must be a number from power of 2", flush=True)
    #     comm.Abort(-1)
    # bits = len(rows_sums) * len(col_sums)
    # if size > bits:
    #     print(f"number of processors {size} must not be greater than number of bits {bits}, consider increasing "
    #           f"matrix size or reduce number of processors", flush=True)
    #     comm.Abort(-1)

    if size > 1:
        def listen_exit():
            # print(rank, "listen exit")
            req = comm.irecv(tag=TAG_DONE)
            while not req.Test():
                # print(rank, "test exit")
                time.sleep(0.3)
            print(rank, "received stop signal", flush=True)
            stop.set()

        def listen_ready(src):
            # print(rank, "listen ready", src)
            while not stop.is_set():
                req = comm.irecv(tag=TAG_READY, source=src)
                while not stop.is_set():
                    # print(rank, "test ready", src)
                    if req.Test():
                        # print(rank, "received ready signal from", src, flush=True)
                        with state:
                            rank_ready[src] = True
                        break
                    else:
                        time.sleep(0.001)

        def listen_busy(src):
            # print(rank, "listen busy", src)
            while not stop.is_set():
                req = comm.irecv(tag=TAG_BUSY, source=src)
                while not stop.is_set():
                    # print(rank, "test busy", src)
                    if req.Test():
                        # print(rank, "received busy signal from", src, flush=True)
                        with state:
                            rank_ready[src] = False
                        break
                    else:
                        time.sleep(0.01)

        threads.submit(listen_exit)
        for r in other_ranks:
            threads.submit(listen_ready, r)
            threads.submit(listen_busy, r)

    start = timer()
    solved = False
    if rank == 0:
        solved = solve(
            matrix=np.zeros(shape=(len(rows_sums), len(col_sums)), dtype=int),
            srow=0,
            scol=0,
            row_sums=np.copy(rows_sums),
            col_sums=np.copy(col_sums),
            static_row_sums=rows_sums,
            static_col_sums=col_sums,
            checksum=matrix_hash,
        )
    while not solved and not stop.is_set():
        did_send = False
        req = comm.irecv(tag=TAG_ADD_WORK)
        work = None
        while not stop.is_set():
            status, work = req.test()
            if not status and not did_send:
                for r in other_ranks:
                    # print(rank, "sending ready signal to rank", r, flush=True)
                    comm.isend(obj=True, dest=r, tag=TAG_READY)
                did_send = True

        if not stop.is_set():
            for r in other_ranks:
                # print(rank, "sending busy signal to rank", r, flush=True)
                comm.isend(obj="BUSY", dest=r, tag=TAG_BUSY)
        if not isinstance(work, dict):
            print(rank, "work invalid type", type(work), work)
            return
        solved = solve(
            matrix=work['next_matrix'],
            srow=work['next_row'],
            scol=work['next_col'],
            row_sums=work['next_row_sums'],
            col_sums=work['next_col_sums'],
            static_row_sums=rows_sums,
            static_col_sums=col_sums,
            checksum=matrix_hash,
        )


def x():
    try:
        main()
    except:
        print(rank, "error", traceback.format_exc(), flush=True)


main()
stop.set()
threads.shutdown()
MPI.Finalize()
print(rank, "program done", flush=True)
