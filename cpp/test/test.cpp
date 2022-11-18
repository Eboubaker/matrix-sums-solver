#include "test.h"
using namespace std;

class MatrixGenerator
{
public:
    MatrixGenerator(t_cell_sum *row_sums, t_cell_sum *col_sums, int nb_rows, int nb_cols) : row_sums(row_sums), col_sums(col_sums), nb_rows(nb_rows), nb_cols(nb_cols), msize(nb_rows * nb_cols)
    {
        current_state = new t_cell[msize]{0};
        open_cells = (int **)malloc(sizeof(int) * nb_rows);
        for (int row = 0; row < nb_rows; row++)
        {
            open_cells[row] = (int *)malloc(sizeof(int) * nb_cols);
            for (int col = 0; col < nb_cols; col++)
            {
                open_cells[row][col] = 1;
            }
        }
        next_state();
    }
    operator bool() { return check_points.size() > 0 || state_zero; }
    t_cell *operator*()
    {
        assert(*this);
        return mtrx_clone(current_state, msize);
    }

    MatrixGenerator &operator++()
    {
        assert(*this);
        next_state();
        return *this;
    }
    MatrixGenerator operator++(int)
    {
        MatrixGenerator tmp = *this;
        ++*this;
        return tmp;
    }

private:
    vector<int> check_points;
    t_cell_sum *row_sums, *col_sums;
    int **open_cells;
    int nb_rows, nb_cols;
    int msize;
    t_cell *current_state;
    bool state_zero = true;
    void next_state()
    {
        int i;
        if (check_points.size() > 0)
        {
            i = check_points.back();
            check_points.pop_back();
            // TODO: fallback to old state
            for (int row = nb_rows - 1; row >= 0; row--)
            {
                for (int col = nb_cols - 1; col >= 0; col--)
                {
                    if (i == col + row * nb_cols)
                    {
                        goto out;
                    }
                    if (current_state[col + row * nb_cols] == 1)
                    {
                        current_state[col + row * nb_cols] = 0;
                    }
                }
            }
        out:
            current_state[i] = 1;
            i++;
        }
        else if (state_zero)
        {
            state_zero = false;
            i = 0;
        }
        else
        {
            cerr << "no more states" << endl;
            exit(1);
        }
        auto crsums = mtrx_sum_rows(current_state, nb_rows, nb_cols);
        auto ccsums = mtrx_sum_cols(current_state, nb_rows, nb_cols);
        for (int row = i / nb_cols; row < nb_rows; row++)
        {
            for (int col = i % nb_cols; col < nb_cols; col++)
            {
                bool zero_set = col_sums[col] == 0 || row_sums[row] == 0 || (true);
                if (zero_set) // we can add 0
                {
                    current_state[col + row * nb_cols] = 0;
                    zero_set = true;
                }
                if (col_sums[col] != 0 && row_sums[row] != 0) // lets see if we can add 1
                {
                    if (zero_set)
                    {
                        check_points.push_back(col + row * nb_cols);
                    }
                    else
                    {
                        current_state[col + row * nb_cols] = 1;
                        crsums[row]++;
                        ccsums[col]++;
                    }
                }
            }
        }
    }
};

int main(int argc, char *argv[])
{
    t_cell mtrx[] = {
        1,
        0,
        0,
        0,
        0,
        0,
        1,
        0,
        1,
        0,
        1,
        0,
        0,
        0,
        1,
    };
    int nb_rows = 3, nb_cols = 5;
    MatrixGenerator gen = MatrixGenerator(mtrx_sum_rows(mtrx, nb_rows, nb_cols), mtrx_sum_cols(mtrx, nb_rows, nb_cols), nb_rows, nb_cols);
    while (gen)
    {
        cout << mtrx_str(*gen, nb_rows, nb_cols) << endl
             << "*******" << endl;
        gen++;
    }

    return 0;
}