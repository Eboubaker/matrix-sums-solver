#include "solver.h"
//#define D_solve 1
t_cell *solve(t_cell_sum *row_sums, t_cell_sum *cols_sums, int nb_rows, int nb_cols, const unsigned char *hash, int part, int parts_count)
{
    int bits_len = nb_cols * nb_rows;
    if (parts_count > bits_len)
    {
        std::cerr << "parts_count > bits_len" << std::endl;
        exit(1);
    }
    t_cell *bits = (t_cell *)malloc(sizeof(t_cell) * bits_len);
    memset(bits, 0, sizeof(t_cell) * bits_len);
    int index;
    int end_bit = bits_len;
    if (part >= parts_count)
    {
        std::cerr << "part index not less than parts count" << std::endl;
        exit(1);
    }
    size_t mask_len = log2(parts_count);
    end_bit = bits_len - mask_len;
    int shift = 0;
    for (size_t bi = end_bit; bi < (size_t)bits_len; bi++, shift++)
    {
        if ((part >> shift) & 1)
        {
            bits[bi] = 1;
        }
    }
    size_t i = 0;
    size_t max = powl(2, end_bit);
    while (i < max)
    {
        for (int row = 0; row < nb_rows; row++)
        {
            if (mtrx_sum_row(bits, row, nb_cols) != row_sums[row])
            {
                goto increment;
            }
        }
        for (int col = 0; col < nb_cols; col++)
        {
            if (mtrx_sum_col(bits, col, nb_rows, nb_cols) != cols_sums[col])
            {
                goto increment;
            }
        }
        if (hash_eq(mtrx_md5dgst(bits, bits_len), hash, MD5_DIGEST_LENGTH))
        {
            return bits;
        }
    increment:
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

int main(int argc, char **argv)
{
    std::vector<std::string> args;
    args.push_back(std::string(argv[0]));
    std::string input;
    if (!isatty(STDIN_FILENO) && std::getline(std::cin, input))
    {
        auto vec = split(input, " ");

        for (size_t i = 0; i < vec.size(); i++)
        {
            args.push_back(vec[i]);
        }
    }
    else
    {
        for (int i = 1; i < argc; i++)
        {
            args.push_back(std::string(argv[i]));
        }
    }
    auto arg1 = args[1];
    auto arg2 = args[2];

    arg1 = arg1.substr(1, arg1.size() - 2);
    arg2 = arg2.substr(1, arg2.size() - 2);

    auto strrows = split(arg1, ",");
    auto strcols = split(arg2, ",");
    int nb_rows = (int)strrows.size();
    int nb_cols = (int)strcols.size();
    int len = MD5_DIGEST_LENGTH * 2;
    unsigned char hextdgst[MD5_DIGEST_LENGTH];

    hextodgst(args[3].c_str(), hextdgst);
    // std::cout << hextdgst << std::endl;

    int row_sums[nb_rows], col_sums[nb_cols];

    for (size_t i = 0; i < strrows.size(); i++)
    {
        row_sums[i] = std::stoi(strrows[i]);
    }
    for (size_t i = 0; i < strcols.size(); i++)
    {

        col_sums[i] = std::stoi(strcols[i]);
    }
    int seed = 0;
    if (args.size() > 4)
    {
        seed = std::stoi(args[4]);
    }
    std::cout << "solving input "
              << int_lst_str(row_sums, nb_rows) << " "
              << int_lst_str(col_sums, nb_cols) << " "
              << dgsttohex(hextdgst, MD5_DIGEST_LENGTH) << " "
              << seed
              << std::endl;
    int processes = std::min(std::max(16, get_num_cores()), (int)powl(2, (int)log2(nb_cols * nb_rows)));
    std::cout << "forking " << processes << " processes on " << get_num_cores() << " processors" << std::endl;
    pid_t workers[processes];
    int channels[processes][2];
    bool parent = true;
    int rank;
    for (int i = 0; i < processes; i++)
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
        for (int i = 0; i < processes; i++)
        {
            if (-1 == write(channels[i][1], workers, sizeof(pid_t) * processes))
            {
                std::cerr << "write to channel " << i << " failed";
                exit(1);
            }
        }
        for (int i = 0; i < processes; i++)
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
        t_cell *s = solve(row_sums, col_sums, nb_rows, nb_cols, hextdgst, rank, processes);
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = (float)std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() / 1000000.0F;

        if (s)
        {
            std::cout << "rank " << rank << " solved in " << std::fixed << duration << "s" << std::endl
                      << mtrx_str(s, nb_rows, nb_cols) << std::endl;
            for (int i = 0; i < processes; i++)
            {
                if (getpid() == brothers[i])
                    continue;
                if (-1 == kill(brothers[i], SIGTERM))
                {
                    // std::cerr << "send SIGTERM from " << getpid() << " (rank " << rank << ") to " << brothers[i] << " failed" << std::endl;
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