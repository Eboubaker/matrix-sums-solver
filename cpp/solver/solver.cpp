#include "solver.h"
//#define D_solve 1
#include <iterator> // for ostream_iterator

std::mutex mtx;
std::condition_variable cond_var;

int count_non_zero(t_cell_sum *sums, int offset, int size)
{
    int nb_non_zero = 0;
    for (int i = offset; i < size; i++)
        if (sums[i] != 0)
            nb_non_zero++;
    return nb_non_zero;
}
t_cell *solve(t_cell *m, int size, int nb_rows, int nb_cols, int s, t_cell_sum *row_sums, t_cell_sum *col_sums, const unsigned char *hash)
{
    int row, col;
    for (int i = s; i < size; i++)
    {
        row = i / nb_cols;
        col = i % nb_cols;
        // if row_sums[row] == 0 or col_sums[col] == 0 or (
        //         np.count_nonzero(col_sums[col:]) > row_sums[row]  # check next columns with non-zero sum in this cell row
        //         and np.count_nonzero(row_sums[row:]) > col_sums[col]  # check next rows with non-zero sum in this cell column
        // )

        if (row_sums[row] == 0 || col_sums[col] == 0 || (count_non_zero(col_sums, col, nb_cols) > row_sums[row] && count_non_zero(row_sums, row, nb_rows) > col_sums[col]))
        {
            if (col_sums[col] != 0 && row_sums[row] != 0)
            {
                t_cell next_matrix[size];
                t_cell_sum next_row_sums[nb_rows];
                t_cell_sum next_col_sums[nb_cols];
                std::copy(m, m + size, next_matrix);
                std::copy(row_sums, row_sums + nb_rows, next_row_sums);
                std::copy(col_sums, col_sums + nb_cols, next_col_sums);

                next_matrix[i] = 1;
                next_col_sums[col]--;
                next_row_sums[row]--;

                t_cell *s = solve(next_matrix, size, nb_rows, nb_cols, i + 1, next_row_sums, next_col_sums, hash);

                if (s)
                    return mtrx_clone(s, size);
            }
        }
        else
        {
            m[i] = 1;
            row_sums[row]--;
            col_sums[col]--;
        }
    }

    // std::cout << "checking " << int_lst_str(m, size) << std::endl;
    if (hash_eq(mtrx_md5dgst(m, size), hash, MD5_DIGEST_LENGTH))
        return m;
    return NULL;
}

//#define TEST_ARG std::string("./solver [2,2] [1,2,1] fdf6d3245193b264adce8118b2d03d60 2341276842")
int main(int argc, char **argv)
{
    std::vector<std::string> args;
#ifdef TEST_ARG
    args = split(TEST_ARG, " ");
#else
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
#endif
    auto strrows = split(args[1].substr(1, args[1].size() - 2), ",");
    auto strcols = split(args[2].substr(1, args[2].size() - 2), ",");
    int nb_rows = (int)strrows.size();
    int nb_cols = (int)strcols.size();
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

    uint32_t seed = 0;
    if (args.size() > 4)
    {
        seed = std::stol(args[4]);
    }

    std::cout << "solving input "
              << int_lst_str(row_sums, nb_rows) << " "
              << int_lst_str(col_sums, nb_cols) << " "
              << dgsttohex(hextdgst, MD5_DIGEST_LENGTH) << " "
              << seed
              << std::endl;
    // int processes = std::min(std::max(16, get_num_cores()), (int)powl(2, (int)log2(nb_cols * nb_rows)));
    std::thread thr([&]()
                {
        int size = nb_rows * nb_cols;
        t_cell m[size] = {0};
        auto start = std::chrono::high_resolution_clock::now();
        auto s = solve(m, size, nb_rows, nb_cols, 0, row_sums, col_sums, hextdgst);
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = (float)std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() / 1000000.0F;

        if (s)
        {
            std::cout << "process " << getpid() << " [main] solved in " << std::fixed << duration << "s" << std::endl
                    << mtrx_str(s, nb_rows, nb_cols) << std::endl;
        } });
    thr.join();

    return 0;
}