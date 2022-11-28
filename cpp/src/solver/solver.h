#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <iostream>
#include <string.h>
#include <cstdlib>
#include <ctime>
#include <math.h>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>

#include <chrono>
#include <atomic>
#include "../include/mtrx_funcs.h"
#include "../include/utils.h"
#define OMPI_SKIP_MPICXX 1
#include "mpi.h"
#include <iterator> // for ostream_iterator
// #include <boost/thread.hpp>
// #include <boost/interprocess/sync/file_lock.hpp>
// #include <boost/interprocess/sync/interprocess_condition.hpp>
// #include <boost/interprocess/sync/scoped_lock.hpp>

#define TAG_TASK 2
#define TAG_EXIT 3
#define TAG_ARGS_COUNT 4
#define TAG_ARG_LEN 5
#define TAG_ARGS 6
#define TAG_ARG 7

t_cell *solve_recursive(t_cell *m, int size, int nb_rows, int nb_cols, int s, t_cell_sum *row_sums, t_cell_sum *col_sums, const unsigned char *hash);