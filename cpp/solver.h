#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <iostream>
#include <string.h>
#include <cstdlib>
#include <ctime>
#include <math.h>
#include "mtrx_funcs.h"
#include "utils.h"

t_cell *solve(t_cell_sum *row_sums, t_cell_sum *cols_sums, int nb_rows, int nb_cols, const unsigned char *hash, int part, int parts_count);