#include "solver.h"
//#define D_solve 1

using namespace std;

int Rank, Size, thread_per_core, *other_ranks;
int pick_send_task_rank()
{
    // return Rank == 0 ? Size - 1 : Rank - 1;
    return other_ranks[rand() % (Size - 1)];
}
template <typename _CharT, typename _Traits>
inline basic_ostream<_CharT, _Traits> &
wexecutor(basic_ostream<_CharT, _Traits> &__os)
{
    return __os << " R[" << Rank << "] P[" << getpid() << "] T[" << this_thread::get_id() << "]" << endl;
}

struct WorkSection
{
    int offset;
    t_cell *matrix;
    t_cell_sum *row_sums, *col_sums;

    WorkSection()
    {
    }
    WorkSection(WorkSection *ws) : offset(ws->offset), matrix(ws->matrix), row_sums(ws->row_sums), col_sums(ws->col_sums)
    {
    }
};
int WorkSection_ints;
int *WorkSection_pack(WorkSection &ws, int nb_rows, int nb_cols)
{
    int *data = (int *)malloc(WorkSection_ints * sizeof(int));
    int s = 0;
    data[s] = ws.offset;
    s++;
    for (int i = 0; i < nb_rows * nb_cols; i++, s++)
    {
        data[s] = ws.matrix[i];
    }
    for (int i = 0; i < nb_rows; i++, s++)
    {
        data[s] = ws.row_sums[i];
    }
    for (int i = 0; i < nb_cols; i++, s++)
    {
        data[s] = ws.col_sums[i];
    }
    assert(WorkSection_ints == s);
    return data;
}
WorkSection *WorkSection_unpack(int *data, int nb_rows, int nb_cols)
{
    WorkSection *ws = (WorkSection *)malloc(sizeof(WorkSection));
    ws->matrix = mtrx_mk(nb_rows * nb_rows);
    ws->row_sums = (t_cell_sum *)malloc(sizeof(t_cell_sum) * nb_rows);
    ws->col_sums = (t_cell_sum *)malloc(sizeof(t_cell_sum) * nb_cols);
    int s = 0;
    ws->offset = data[s];
    s++;
    for (int i = 0; i < nb_rows * nb_cols; i++, s++)
    {
        ws->matrix[i] = data[s];
    }
    for (int i = 0; i < nb_rows; i++, s++)
    {
        ws->row_sums[i] = data[s];
    }
    for (int i = 0; i < nb_cols; i++, s++)
    {
        ws->col_sums[i] = data[s];
    }
    assert(s == WorkSection_ints);
    return ws;
}

atomic<bool> stop;
int count_non_zero(t_cell_sum *sums, int offset, int size)
{
    int nb_non_zero = 0;
    for (int i = offset; i < size; i++)
        if (sums[i] != 0)
            nb_non_zero++;
    return nb_non_zero;
}
vector<WorkSection *> q;
mutex qm;
t_cell *solve_recursive_wthreading(t_cell *m, int size, int nb_rows, int nb_cols, int s, t_cell_sum *row_sums, t_cell_sum *col_sums, const unsigned char *hash)
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
                WorkSection *ws = (WorkSection *)malloc(sizeof(WorkSection));
                ws->matrix = mtrx_mk(size);
                ws->row_sums = (t_cell_sum *)malloc(sizeof(t_cell_sum) * nb_rows);
                ws->col_sums = (t_cell_sum *)malloc(sizeof(t_cell_sum) * nb_cols);
                // if(Rank == 0){
                //     cout << "1:" << ws->matrix << "->" << ws->matrix + size << " 2:" << ws->row_sums << "->" << ws->row_sums + nb_rows << endl;
                // }
                copy(m, m + size, ws->matrix);
                copy(row_sums, row_sums + nb_rows, ws->row_sums);
                copy(col_sums, col_sums + nb_cols, ws->col_sums);

                ws->matrix[i] = 1;
                ws->offset = i + 1;
                ws->row_sums[row]--;
                ws->col_sums[col]--;
                unique_lock<mutex> lk(qm);
                if (q.size() > 0)
                {
                    lk.unlock();
                    int *data = WorkSection_pack(*ws, nb_rows, nb_cols);
                    // int dest = other_ranks[rand() % (Size - 1)]; // random rank
                    int dest = pick_send_task_rank();
                    // cout << "Rank " << Rank << " Sending to " << dest << " " << int_lst_str(ws.matrix, size) << endl;
                    // cout << "put" << endlrank;
                    MPI_Send(data, WorkSection_ints, MPI_INT, dest, TAG_TASK, MPI_COMM_WORLD);
                    free(data);
                    free(ws->matrix);
                    free(ws->row_sums);
                    free(ws->col_sums);
                    free(ws);
                }
                else
                {
                    q.push_back(ws);
                    lk.unlock();
                }
            }
        }
        else
        {
            m[i] = 1;
            row_sums[row]--;
            col_sums[col]--;
        }
    }

    // cout << "Rank " << Rank << " checking " << int_lst_str(m, size) << endl;

    if (hash_eq(mtrx_md5dgst(m, size), hash, MD5_DIGEST_LENGTH))
        return mtrx_clone(m, size);
    // cout << "Rank " << Rank << " end checking " << endl;
    return NULL;
}

t_cell *solve_recursive(t_cell *m, int size, int nb_rows, int nb_cols, int s, t_cell_sum *row_sums, t_cell_sum *col_sums, const unsigned char *hash)
{
    int row, col;
    for (int i = s; i < size; i++)
    {
        row = i / nb_cols;
        col = i % nb_cols;

        if (row_sums[row] == 0 || col_sums[col] == 0)
            continue;
        if (count_non_zero(col_sums, col, nb_cols) > row_sums[row] && count_non_zero(row_sums, row, nb_rows) > col_sums[col])
        {
            WorkSection ws;
            ws.matrix = mtrx_mk(size);
            ws.row_sums = (t_cell_sum *)malloc(sizeof(t_cell_sum) * nb_rows);
            ws.col_sums = (t_cell_sum *)malloc(sizeof(t_cell_sum) * nb_cols);
            copy(m, m + size, ws.matrix);
            copy(row_sums, row_sums + nb_rows, ws.row_sums);
            copy(col_sums, col_sums + nb_cols, ws.col_sums);

            ws.matrix[i] = 1;
            ws.offset = i + 1;
            ws.row_sums[row]--;
            ws.col_sums[col]--;
            if (i * 1.0f / size < .2) // naive approach to balance the workload, only send before we reach 20%
            {
                int *data = WorkSection_pack(ws, nb_rows, nb_cols);
                // int dest = other_ranks[rand() % (Size - 1)]; // random rank
                int dest = pick_send_task_rank();
                // cout << "Rank " << Rank << " Sending to " << dest << " " << int_lst_str(ws.matrix, size) << endl;
                // cout << "put" << endlrank;
                MPI_Send(data, WorkSection_ints, MPI_INT, dest, TAG_TASK, MPI_COMM_WORLD);
                free(data);
                free(ws.matrix);
                // ws.matrix = NULL;
                free(ws.row_sums);
                // ws.row_sums = NULL;
                free(ws.col_sums);
                // ws.col_sums = NULL;
            }
            else
            {
                auto m = solve_recursive(ws.matrix, size, nb_rows, nb_cols, ws.offset, ws.row_sums, ws.col_sums, hash);
                if (m != NULL)
                {
                    return m;
                }
            }
        }
        else
        {
            m[i] = 1;
            row_sums[row]--;
            col_sums[col]--;
        }
    }

    // cout << "Rank " << Rank << " checking " << int_lst_str(m, size) << endl;

    if (hash_eq(mtrx_md5dgst(m, size), hash, MD5_DIGEST_LENGTH))
        return mtrx_clone(m, size);
    // cout << "Rank " << Rank << " end checking " << endl;
    return NULL;
}
atomic<int> waiters(0);
void parallel_solve_wthreading(int nb_rows, int nb_cols, const unsigned char *hashdigest)
{
    // cout << " START " << Rank << endl;
    auto tstart = chrono::high_resolution_clock::now();
    int size = nb_rows * nb_cols;
    condition_variable cv;
    while (1)
    {
        WorkSection *ws = (WorkSection *)malloc(sizeof(WorkSection));
        unique_lock<mutex> lk(qm);
        if (q.size() == 0)
        {
            if (waiters < thread_per_core - 1)
            {
                waiters++;
                cv.wait(lk);
                waiters--;
                ws = q.back();
                q.pop_back();
                lk.unlock();
            }
            else
            {
                lk.unlock();
                int *recv = (int *)malloc(sizeof(int) * WorkSection_ints);
                MPI_Recv(recv, WorkSection_ints, MPI_INT, MPI_ANY_SOURCE, TAG_TASK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                ws = WorkSection_unpack(recv, nb_rows, nb_cols);
                free(recv);
            }
        }
        else
        {
            ws = q.back();
            q.pop_back();
            lk.unlock();
        }
        auto m = solve_recursive_wthreading(ws->matrix, size, nb_rows, nb_cols, ws->offset, ws->row_sums, ws->col_sums, hashdigest);
        // if (Rank == 0)
        //     cout << "FREE " << ws->matrix << "->" << ws->matrix + nb_cols * nb_rows << wexecutor;
        free(ws->matrix);
        // ws->matrix = NULL;
        // if (Rank == 0)
        //     cout << "FREE2 " <<  ws->row_sums  << "->" << ws->row_sums + nb_rows << " "  << int_lst_str(ws->row_sums, nb_cols) << wexecutor;

        // free(ws->row_sums);
        // ws->row_sums = NULL;
        // if (Rank == 0)
        //     cout << "FREE3 " << int_lst_str(ws->col_sums, nb_cols) << wexecutor;
        // free(ws->col_sums);
        // ws->col_sums = NULL;
        // if (Rank == 0)
        //     cout << "FREE4 " << ws << wexecutor;
        free(ws);
        // ws = NULL;
        if (m)
        {
            // pidlog("found solution");
            auto tstop = chrono::high_resolution_clock::now();
            auto duration = (float)chrono::duration_cast<chrono::microseconds>(tstop - tstart).count() / 1000000.0F;
            cout << "rank " << Rank << " solved after " << duration << "s" << endl
                 << mtrx_str(m, nb_rows, nb_cols) << endl;
            MPI_Abort(MPI_COMM_WORLD, 0);
            // int _ = 0;
            // for (int i = 0; i < size - 1; i++)
            //     MPI_Send(&_, 1, MPI_INT, other_ranks[i], TAG_EXIT, MPI_COMM_WORLD);
            // stop = true;
        }
        // pidlog("end consume");
    }
}
void parallel_solve(int nb_rows, int nb_cols, const unsigned char *hashdigest)
{
    // cout << " START " << Rank << endl;
    auto tstart = chrono::high_resolution_clock::now();
    int size = nb_rows * nb_cols;
    while (1)
    {
        // cout << "read" << endlrank;
        // #print(f "{rank} wait task tag {TAG_TASK}", flush = True)
        // MPI_Status status;
        int *recv = (int *)malloc(sizeof(int) * WorkSection_ints);
        MPI_Recv(recv, WorkSection_ints, MPI_INT, MPI_ANY_SOURCE, TAG_TASK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        // cout << "consume" << endlrank;
        // cout << "T: " << t;
        // cout.flush();
        // Treq_set = true;
        // if (MPI_SUCCESS != MPI_Wait(&Treq, MPI_STATUS_IGNORE))
        // {
        //     cout << " ERROR TASK " << endl;
        //     break;
        // }
        WorkSection *ws = WorkSection_unpack(recv, nb_rows, nb_cols);

        // cout << "Rank " << Rank << " Recivied from " << status.MPI_SOURCE << " " << int_lst_str(ws->matrix, size) << endl;
        free(recv);
        // #print(f "{rank} got new task")
        auto m = solve_recursive(ws->matrix, size, nb_rows, nb_cols, ws->offset, ws->row_sums, ws->col_sums, hashdigest);
        free(ws->matrix);
        free(ws->row_sums);
        free(ws->col_sums);
        free(ws);
        if (m)
        {
            // pidlog("found solution");
            auto tstop = chrono::high_resolution_clock::now();
            auto duration = (float)chrono::duration_cast<chrono::microseconds>(tstop - tstart).count() / 1000000.0F;
            cout << "rank " << Rank << " solved after " << duration << "s" << endl
                 << mtrx_str(m, nb_rows, nb_cols) << endl;
            MPI_Abort(MPI_COMM_WORLD, 0);
            // int _ = 0;
            // for (int i = 0; i < size - 1; i++)
            //     MPI_Send(&_, 1, MPI_INT, other_ranks[i], TAG_EXIT, MPI_COMM_WORLD);
            // stop = true;
        }
        // pidlog("end consume");
    }
}

#define TEST_ARG string("./solver [2,2] [1,2,1] fdf6d3245193b264adce8118b2d03d60 2341276842")
int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &Size);
    MPI_Comm_rank(MPI_COMM_WORLD, &Rank);
    // thread_per_core = boost::thread::hardware_concurrency() / boost::thread::physical_concurrency();
    // if (Rank == 0 && Size > (int)boost::thread::physical_concurrency())
    // {
    //     printf("Warning: running mpi on single machine with number of ranks exceeding number of cores might slow down computation.\n");
    // }
    other_ranks = (int *)malloc(sizeof(int) * (Size - 1));
    int j = 0;
    for (int i = 0; i < Size; i++)
    {
        if (i == Rank)
            continue;
        other_ranks[j] = i;
        j++;
    }
#if 0
    args = split(TEST_ARG, " ");
#else
    vector<string> args;
    if (Rank == 0)
    {
        args.push_back(string(argv[0]));
        string input;

        if (argc == 1 && getline(cin, input))
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
        if (args.size() == 4)
        {
            args.push_back("0");
        }
        int count = (int)args.size();
        // cout << "LEN " << MD5_DIGEST_LENGTH << endl;

        for (int i = 0; i < Size - 1; i++)
        {
            MPI_Send(&count, 1, MPI_INT, other_ranks[i], TAG_ARGS_COUNT, MPI_COMM_WORLD);
            for (auto arg : args)
            {
                auto send = arg.c_str();
                int size = strlen(send) + 1;
                // cout << "send size " << size << endl;
                // cout << "send " << send << endl;
                MPI_Send(&size, 1, MPI_INT, other_ranks[i], TAG_ARG_LEN, MPI_COMM_WORLD);
                MPI_Send(send, size, MPI_CHAR, other_ranks[i], TAG_ARG, MPI_COMM_WORLD);
            }
        }
    }
    else
    {
        // cout << "start recv" << endlrank;
        int count;
        MPI_Recv(&count, 1, MPI_INT, 0, TAG_ARGS_COUNT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        // cout << "recived" << endlrank;
        // cout << "begin add " << count << endlrank;

        for (int i = 0; i < count; i++)
        {
            int size;
            MPI_Recv(&size, 1, MPI_INT, 0, TAG_ARG_LEN, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // cout << "next size " << size << endlrank;
            char data[size];
            MPI_Recv(data, size, MPI_CHAR, 0, TAG_ARG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // cout << "next " << data << endlrank;
            args.push_back(string(data));
        }
        // cout << "end recv" << endlrank;
    }
#endif
    MPI_Barrier(MPI_COMM_WORLD);
    // cout << "strrows " << args[1] << endlrank;
    // cout << "strcols " << args[2] << endlrank;
    // cout << "hash " << args[3] << " " << args[3].length() <<  endlrank;
    // cout << "seed " << args[4] << endlrank;

    auto strrows = split(args[1].substr(1, args[1].size() - 2), ",");
    auto strcols = split(args[2].substr(1, args[2].size() - 2), ",");
    int nb_rows = (int)strrows.size();
    int nb_cols = (int)strcols.size();
    WorkSection_ints = 1 + nb_rows * nb_cols + nb_rows + nb_cols;
    unsigned char hexdgst[MD5_DIGEST_LENGTH];
    hextodgst(args[3].c_str(), hexdgst);

    int row_sums[nb_rows], col_sums[nb_cols];

    for (size_t i = 0; i < strrows.size(); i++)
    {
        row_sums[i] = stoi(strrows[i]);
    }

    for (size_t i = 0; i < strcols.size(); i++)
    {
        // if (Rank == 1)
        // {
        // cout << "next i: " << i << endlrank;
        // cout << "convert: " << strcols[i] << endlrank;
        // cout << "start: " << strcols[i] << endlrank;
        // }
        col_sums[i] = stoi(strcols[i]);
        // if (Rank == 1)
        // {
        // cout << "end: " << strcols[i] << endlrank;
        // }
    }
    // if (Rank == 1)
    // {
    // cout << "end cols" << endlrank;
    // cout << "seed: " << args[4] << endlrank;
    // }
    uint32_t seed = 0;
    if (args.size() > 4)
    {
        seed = stol(args[4]);
    }
    // if (Rank == 1)
    // {
    // cout << "end-seed: " << endlrank;
    MPI_Barrier(MPI_COMM_WORLD);

    // }
    // MPI_Finalize();
    // exit(0);
    // return 0;
    if (Size < 2)
    {
        fprintf(stderr, "Requires at least two processes.\n");
        exit(-1);
    }

    if (Rank == 0)
    {
        cout << "solving input "
             << int_lst_str(row_sums, nb_rows) << " "
             << int_lst_str(col_sums, nb_cols) << " "
             << dgsttohex(hexdgst, MD5_DIGEST_LENGTH);
        if (seed != 0)
            cout << " " << seed;
        cout << endl;
        WorkSection ws;
        ws.matrix = mtrx_mk(nb_rows * nb_cols);
        memset(ws.matrix, 0, sizeof(t_cell) * nb_rows * nb_cols);
        ws.row_sums = row_sums;
        ws.col_sums = col_sums;
        ws.offset = 0;
        // cout << "Rank " << Rank << " Send initial to " << other_ranks[0] << endl;
        // pidlog("put");
        int d = pick_send_task_rank();
        MPI_Send(WorkSection_pack(ws, nb_rows, nb_cols), WorkSection_ints, MPI_INT, d, TAG_TASK, MPI_COMM_WORLD);
    }
    if (/*thread_per_core > 1*/ 0)
    {
        thread threads[thread_per_core];
        for (int i = 0; i < thread_per_core; i++)
        {
            threads[i] = thread(parallel_solve_wthreading, nb_rows, nb_cols, hexdgst);
        }
        for (int i = 0; i < thread_per_core; i++)
        {
            if (threads[i].joinable())
            {
                threads[i].join();
            }
        }
    }
    else
    {
        parallel_solve(nb_rows, nb_cols, hexdgst);
    }
    // thread t = thread(parallel_solve, nb_rows, nb_cols, hexdgst);
    // MPI_Request req;
    // MPI_Status status;
    // int _;
    // int *exit = new int(0);
    // MPI_Irecv(&_, 1, MPI_INT, MPI_ANY_SOURCE, TAG_EXIT, MPI_COMM_WORLD, &req);
    // do
    // {
    //     cout << "TEST: " << MPI_Test(&req, exit, &status);
    //     this_thread::sleep_for(200ms);
    // } while (!stop && !*exit && !status.MPI_ERROR);
    // stop = true;
    // if (Treq_set)
    //     MPI_Cancel(&Treq);
    // if (t.joinable())
    //     t.join();
    // pidlog("left");
    return 0;
}