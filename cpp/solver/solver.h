#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <iostream>
#include <string.h>
#include <cstdlib>
#include <ctime>
#include <math.h>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <queue>
#include "../include/mtrx_funcs.h"
#include "../include/utils.h"
t_cell *solve(t_cell *m, int size, int nb_rows, int nb_cols, int s, t_cell_sum *row_sums, t_cell_sum *col_sums, const unsigned char *hash);