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

//#define D_solve 0
t_cell *solve(t_cell_sum *row_sums, t_cell_sum *cols_sums, int nb_rows, int nb_cols, const unsigned char *hash, int part, int parts_count)
{
    size_t bits_len = nb_cols * nb_rows;
    if (parts_count > bits_len)
    {
        std::cerr << "parts_count > bits_len" << std::endl;
        exit(1);
    }
    t_cell* bits = mtrx_mk(bits_len);
    mtrx_rearrange(bits, new t_cell[bits_len]{0}, bits_len);// fill(0)
    size_t i = 0;
    int index;
    bool valid;
    t_cell_sum *rsums, *csums;
    int end_bit = bits_len;
    if (part >= parts_count)
    {
        std::cerr << "part index not less than parts count" << std::endl;
        exit(1);
    }
    size_t mask_len = log2(parts_count);
    end_bit = bits_len - mask_len;
    int shift = 0;
    for (size_t bi = end_bit; bi < bits_len; bi++)
    {
        if ((part >> shift) & 1)
        {
            bits[bi] = 1;
        }
        shift++;
    }
    size_t max = powl(2, end_bit);
    while (i < max)
    {
        // std::cout << getpid() << " " << int_lst_str(bits, bits_len) << std::endl;
        valid = true;
#ifdef D_solve
        std::cout << mtrx_str(m, nb_rows, nb_cols) << std::endl;
#endif
        rsums = mtrx_sum_rows(bits, nb_rows, nb_cols);
        csums = mtrx_sum_cols(bits, nb_rows, nb_cols);
#ifdef D_solve
        std::cout << "rows:" << int_lst_str(rsums, nb_rows) << std::endl;
        std::cout << "cols:" << int_lst_str(csums, nb_cols) << std::endl;
#endif
        for (size_t row = 0; row < nb_rows; row++)
        {
            if (rsums[row] != row_sums[row])
            {
                valid = false;
                break;
            }
        }
        for (size_t col = 0; col < nb_cols; col++)
        {
            if (csums[col] != cols_sums[col])
            {
                valid = false;
                break;
            }
        }
        free(rsums);
        free(csums);
#ifdef D_solve
        std::cout << "v: " << valid << std::endl;
#endif
        if (valid && hash_eq(mtrx_md5dgst(bits, bits_len), hash, MD5_DIGEST_LENGTH))
        {
#ifdef D_solve
            std::cout << "Solve: " << m << ":" << mtrx_str(m, nb_rows, nb_cols) << std::endl;
#endif
            t_cell *r = mtrx_clone(bits, bits_len);
            free(bits);
            return r;
        }
#ifdef D_solve
        std::cout << "--" << std::endl;
#endif
        if (i < max)
        {
            index = 0;
            bits[index]++;
            while (bits[index] == 2)
            {
                bits[index] = 0;
                bits[++index]++;
            }
        }
        i++;
    }
    return NULL;
}

int main()
{
    srand(time(0));
    int nb_rows = 3;
    int nb_cols = 3;
    // srand(4);
    t_cell *m = mtrx_mk(nb_rows * nb_cols);
    mtrx_rnd(m, nb_rows * nb_cols);
    std::cout << mtrx_str(m, nb_rows, nb_cols) << std::endl;
    int processes = std::min(32, (int)powl(2, (int)log2(nb_cols * nb_rows)));
    std::cout << "forking " << processes << " processes on " << get_num_cores() << " processors" << std::endl;
    // for (size_t i = 0; i < processes; i++)
    // {
    // int pipefd[2] = {0};
    // if (-1 == pipe(pipefd))
    // {
    //     std::cout << "pip create fail" << std::endl;
    //     exit(1);
    // }
    // write(pipefd[1], &i, sizeof(int));
    pid_t workers[processes];
    int channels[processes][2];
    bool parent = true;
    int rank;
    for (size_t i = 0; i < processes; i++)
    {
        pipe(channels[i]);
        pid_t cpid = fork();
        if (cpid == 0) // child
        {
            parent = false;
            rank = i;
            break;
        }
        else
        {
            workers[i] = cpid;
        }
    }
    if (parent)
    {
        for (size_t i = 0; i < processes; i++)
        {
            if (-1 == write(channels[i][1], workers, sizeof(pid_t) * processes))
            {
                std::cerr << "write to channel " << i << " failed";
                exit(1);
            }
        }
        for (size_t i = 0; i < processes; i++)
        {
            int status;
            while (-1 == waitpid(workers[i], &status, 0))
                ;
            // if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
            // {
            //     std::cerr << "Process " << i << " (pid " << workers[i] << ") failed" << std::endl;
            //     exit(1);
            // }
        }
    }
    else
    {
        pid_t brothers[processes];
        if (-1 == read(channels[rank][0], brothers, sizeof(pid_t) * processes))
        {
            std::cerr << "read frmo channel " << rank << " failed";
            exit(1);
        }
        // std::cout << "rank " << rank << " works on " << rank << std::endl;
        auto start = std::chrono::high_resolution_clock::now();
        t_cell *s = solve(mtrx_sum_rows(m, nb_rows, nb_cols), mtrx_sum_cols(m, nb_rows, nb_cols), nb_rows, nb_cols, mtrx_md5dgst(m, nb_rows * nb_cols), rank, processes);
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = (float)std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() / 1000000.0F;

        if (s)
        {
            std::cout << "rank " << rank << " solved in " << std::fixed << duration << "s" << std::endl
                      << mtrx_str(s, nb_rows, nb_cols) << std::endl;
            for (size_t i = 0; i < processes; i++)
            {
                if (getpid() == brothers[i])
                    continue;
                if (-1 == kill(brothers[i], SIGTERM))
                {
                    //std::cerr << "send SIGTERM from " << getpid() << " (rank " << rank << ") to " << brothers[i] << " failed" << std::endl;
                }
            }
        }
        // else
        // {
        //     std::cout << "rank " << rank << " not solved " << std::endl;
        // }
    }
    // }
    return 0;
}