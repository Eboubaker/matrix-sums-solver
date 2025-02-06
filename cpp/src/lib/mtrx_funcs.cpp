#include "stdio.h"
#include <stdlib.h>
#include <sstream>
#include <iostream>
#include <string.h>
#include <cstdlib>
#include <ctime>

#include "openssl/md5.h"
#include "mtrx_funcs.h"
#include "utils.h"
#include <cassert>

const unsigned char *mtrx_md5dgst(t_cell *m, int size)
{
    MD5_CTX md5_ctx;
    MD5_Init(&md5_ctx);
    for (int i = 0; i < size; i++)
    {
        MD5_Update(&md5_ctx, (void *)(m+i), sizeof(t_cell));
    }
    unsigned char *digest = (unsigned char *)malloc(sizeof(unsigned char) * MD5_DIGEST_LENGTH);
    MD5_Final(digest, &md5_ctx);
    return digest;
}

std::string mtrx_str(t_cell *matrix, int nb_rows, int nb_cols)
{
    std::stringstream ss;
    ss << "[";
    for (int row = 0; row < nb_rows; row++)
    {
        if (row != 0)
        {
            ss << "\n";
        }
        ss << "[";
        if (nb_cols > 0)
        {
            ss << matrix[row * nb_cols];
        }
        for (int col = 1; col < nb_cols; col++)
        {
            ss << "," << matrix[col + row * nb_cols];
        }
        ss << "]";
    }
    ss << "]";
    return ss.str();
}

t_cell *mtrx_mk(int size)
{
    t_cell *matrix = (t_cell *)malloc(sizeof(t_cell) * size);
    return matrix;
}
void mtrx_rearrange(t_cell *m, t_cell *values, int size)
{
    for (int i = 0; i < size; i++){
        m[i] = values[i];
    }
}
t_cell cell_rnd()
{
    return rand() % 2;
}

t_cell cell_rnd(float fill_factor)
{
    return (float)rand() / (float)RAND_MAX < fill_factor;
}
t_cell *mtrx_clone(t_cell *m, int size)
{
    t_cell *matrix = mtrx_mk(size);

    for (int i = 0; i < size; i++)
    {
        matrix[i] = m[i];
    }
    return matrix;
}
void mtrx_rnd(t_cell *m, int size)
{
    for (int i = 0; i < size; i++)
    {
        m[i] = cell_rnd();
    }
}
void mtrx_rnd(t_cell *m, int size, float fill_factor)
{
    for (int i = 0; i < size; i++)
    {
        m[i] = cell_rnd(fill_factor);
    }
}

t_cell *mtrx_chnk(t_cell *m, int *row_chunks, int *col_chunks, int nb_rows, int nb_cols, int new_nb_rows, int new_nb_cols)
{
    // std::cout << "col chunks: " << new_nb_cols << ":" << sums_str(col_chunks, new_nb_cols) << std::endl;
    // std::cout << "row chunks: " << new_nb_rows << ":" << sums_str(row_chunks, new_nb_rows) << std::endl;
    // std::cout << "matrix " << mtrx_str(m, nb_rows, nb_cols) << std::endl;
    t_cell *new_matrix = (t_cell *)malloc(sizeof(t_cell ) * new_nb_rows * new_nb_cols);
    int nrow = 0, ncol = 0;
    for (int row = 0; row < nb_rows; row++)
    {
        if (!exists(row, row_chunks, new_nb_rows))
            continue;
        assert(nrow <= nb_rows);
        // std::cout << "v" << *new_matrix[row][0] << std::endl;
        // exit(0);
        for (int col = 0; col < nb_cols; col++)
        {
            if (!exists(col, col_chunks, new_nb_cols))
                continue;
            assert(ncol <= nb_cols);
            new_matrix[ncol + nrow * new_nb_cols] = m[col + row * nb_cols];
            ncol++;
        }
        ncol = 0;
        nrow++;
    }
    return new_matrix;
}

t_cell_sum mtrx_sum_row(t_cell *m, int row, int nb_cols)
{
    t_cell_sum sum = 0;
    int shift = row * nb_cols;
    for (int col = 0; col < nb_cols; col++)
    {
        sum += m[col + shift];
    }
    return sum;
}

t_cell_sum *mtrx_sum_rows(t_cell *m, int nb_rows, int nb_cols)
{
    t_cell_sum *sums = (t_cell_sum *)malloc(sizeof(t_cell_sum) * nb_rows);
    for (int row = 0; row < nb_rows; row++)
    {
        sums[row] = mtrx_sum_row(m, row, nb_cols);
    }
    return sums;
}

t_cell_sum *mtrx_sum_cols(t_cell *m, int nb_rows, int nb_cols)
{
    t_cell_sum *sums = (t_cell_sum *)malloc(sizeof(t_cell_sum) * nb_cols);
    for (int col = 0; col < nb_cols; col++)
    {
        sums[col] = mtrx_sum_col(m, col, nb_rows, nb_cols);
    }
    return sums;
}

t_cell_sum mtrx_sum_col(t_cell *m, int col, int nb_rows, int nb_cols)
{
    t_cell_sum sum = 0;
    for (int row = 0; row < nb_rows; row++)
    {
        sum += m[col + row * nb_cols];
    }
    return sum;
}

std::string sums_str(t_cell_sum *sums, int count)
{
    std::stringstream ss;
    ss << "[";
    for (int i = 0; i < count; i++)
    {
        if (i != 0)
        {
            ss << ",";
        }
        ss << (int)sums[i];
    }
    ss << "]";
    return ss.str();
}
