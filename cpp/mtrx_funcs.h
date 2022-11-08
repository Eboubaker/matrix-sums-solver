
#include "openssl/md5.h"

typedef int t_cell;
typedef int t_cell_sum;

const unsigned char *mtrx_md5dgst(t_cell *m, int size);
void mtrx_rnd(t_cell *m, int size);
t_cell cell_rnd();
t_cell *cell_mk(t_cell value);
t_cell *mtrx_clone(t_cell *m, int size);

t_cell *mtrx_chnk(t_cell *m, int *row_chunks, int *col_chunks, int nb_rows, int nb_cols, int new_nb_rows, int new_nb_cols);
t_cell *mtrx_mk(int size);
void mtrx_rearrange(t_cell *m, t_cell *values, int size);
t_cell_sum mtrx_sum_row(t_cell *m, int row, int nb_cols);
t_cell_sum mtrx_sum_col(t_cell *m, int col, int nb_rows, int nb_cols);
t_cell_sum *mtrx_sum_cols(t_cell *m, int nb_rows, int nb_cols);
t_cell_sum *mtrx_sum_rows(t_cell *m, int nb_rows, int nb_cols);
std::string mtrx_str(t_cell *m, int nb_rows, int nb_cols);