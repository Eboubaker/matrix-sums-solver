#include "solver.h"
//#define D_solve 1
#include <iterator> // for ostream_iterator

using namespace std;

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

        if (row_sums[row] == 0 || col_sums[col] == 0 || (count_non_zero(col_sums, col, nb_cols) > row_sums[row] && count_non_zero(row_sums, row, nb_rows) > col_sums[col]))
        {
            if (col_sums[col] != 0 && row_sums[row] != 0)
            {
                t_cell next_matrix[size];
                t_cell_sum next_row_sums[nb_rows];
                t_cell_sum next_col_sums[nb_cols];
                copy(m, m + size, next_matrix);
                copy(row_sums, row_sums + nb_rows, next_row_sums);
                copy(col_sums, col_sums + nb_cols, next_col_sums);

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

    // cout << "checking " << int_lst_str(m, size) << endl;
    if (hash_eq(mtrx_md5dgst(m, size), hash, MD5_DIGEST_LENGTH))
        return m;
    return NULL;
}
struct WorkSection
{
    int offset;
    t_cell_sum *row_sums, *col_sums;

    WorkSection(int offset, t_cell_sum *row_sums, t_cell_sum *col_sums): offset(offset), row_sums(row_sums), col_sums(col_sums)
    {}
};

class Work
{
public:
    Work(Work &wrk)
    {
    }
    Work(vector<Work> workers, queue<WorkSection> wq, shared_ptr<mutex> mtx, shared_ptr<condition_variable> cond_var, int nb_rows, int nb_cols, const unsigned char *hexdgst)
        : workers(workers), nb_rows(nb_rows), nb_cols(nb_cols), hexdgst(hexdgst), mtx(mtx), cond_var(cond_var), wq(wq)
    {
    }
    void start()
    {
        this->task_ = thread(&Work::worker_function, this);
        // this->task_.detach(); <<<<<< Don't do that.
    }

    void stop()
    {
        this->isRunning_ = false;
        task_.join(); // <<<  Instead of detaching the thread, join() it.
    }
    ~Work()
    {
        this->stop();
    }

private:
    atomic<bool> isRunning_;
    thread task_;
    vector<Work> workers;
    shared_ptr<mutex> mtx;
    shared_ptr<condition_variable> cond_var;
    int nb_rows;
    int nb_cols;
    const unsigned char *hexdgst;
    queue<WorkSection> wq;

    void worker_function()
    {
        int size = nb_rows * nb_cols;
        t_cell m[size] = {0};
        auto start = chrono::high_resolution_clock::now();
        typeof start now;
        this->isRunning_ = true;
        while (this->isRunning_)
        {
            unique_lock<mutex> lock(*mtx.get());
            do
            {
                now = std::chrono::system_clock::now();
            } while (cv_status::timeout == cond_var.get()->wait_until(lock, now + 100ms) && this->isRunning_);
            if (!this->isRunning_)
                return;
            auto w = wq.back();
            wq.pop();
            auto s = solve(m, size, nb_rows, nb_cols, w.offset, w.row_sums, w.col_sums, hexdgst);
            if (s)
            {
                auto stop = chrono::high_resolution_clock::now();
                auto duration = (float)chrono::duration_cast<chrono::microseconds>(stop - start).count() / 1000000.0F;
                cout << "worker " << this_thread::get_id() << " solved in " << fixed << duration << "s" << endl
                     << mtrx_str(s, nb_rows, nb_cols) << endl;

                return;
            }
        }
    }
};
//#define TEST_ARG string("./solver [2,2] [1,2,1] fdf6d3245193b264adce8118b2d03d60 2341276842")
int main(int argc, char **argv)
{
    vector<string> args;
#ifdef TEST_ARG
    args = split(TEST_ARG, " ");
#else
    args.push_back(string(argv[0]));
    string input;

    if (!isatty(STDIN_FILENO) && getline(cin, input))
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
            args.push_back(string(argv[i]));
        }
    }
#endif
    auto strrows = split(args[1].substr(1, args[1].size() - 2), ",");
    auto strcols = split(args[2].substr(1, args[2].size() - 2), ",");
    int nb_rows = (int)strrows.size();
    int nb_cols = (int)strcols.size();
    unsigned char hexdgst[MD5_DIGEST_LENGTH];

    hextodgst(args[3].c_str(), hexdgst);
    // cout << hextdgst << endl;

    int row_sums[nb_rows], col_sums[nb_cols];

    for (size_t i = 0; i < strrows.size(); i++)
    {
        row_sums[i] = stoi(strrows[i]);
    }
    for (size_t i = 0; i < strcols.size(); i++)
    {

        col_sums[i] = stoi(strcols[i]);
    }

    uint32_t seed = 0;
    if (args.size() > 4)
    {
        seed = stol(args[4]);
    }

    cout << "solving input "
         << int_lst_str(row_sums, nb_rows) << " "
         << int_lst_str(col_sums, nb_cols) << " "
         << dgsttohex(hexdgst, MD5_DIGEST_LENGTH) << " "
         << seed
         << endl;
    // int processes = min(max(16, get_num_cores()), (int)powl(2, (int)log2(nb_cols * nb_rows)));
    vector<Work> workers;
    int wsize = get_num_cores();
    shared_ptr<mutex> mtx(new mutex());
    shared_ptr<condition_variable> cond_var(new condition_variable());
    queue<struct WorkSection> wq;

    for (int i = 0; i < wsize; i++)
        workers.push_back(Work(workers, wq, mtx, cond_var, nb_rows, nb_cols, hexdgst));
    t_cell m[nb_rows * nb_cols] = {0};
        for (int i = 0; i < wsize; i++)
        workers.push_back(Work(workers, wq, mtx, cond_var, nb_rows, nb_cols, hexdgst));
    mtx.get()->lock();
    wq.push(WorkSection(0, row_sums, col_sums));
    cond_var.get()->notify_one();
    mtx.get()->unlock();
    return 0;
}