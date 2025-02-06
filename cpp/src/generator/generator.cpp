#include "generator.h"
int main(int argc, char **argv)
{
    float fill_factor;
    if (argc > 3)
    {
        fill_factor = std::stof(argv[3]);
    }
    else
    {
        fill_factor = 0.5f;
    }
    uint32_t seed;
    if (argc > 4)
    {
        seed = std::stol(argv[4]);
    }
    else
    {
        seed = millis();
    }
    srand(seed);
    int nb_rows = std::stoi(argv[1]);
    int nb_cols = std::stoi(argv[2]);
    int mtrx_s = nb_rows * nb_cols;
    t_cell m[mtrx_s];
    mtrx_rnd(m, mtrx_s, fill_factor);
    if (argc > 4)
    {
        std::cout << mtrx_str(m, nb_rows, nb_cols) << std::endl;
    }

    std::cout << sums_str(mtrx_sum_rows(m, nb_rows, nb_cols), nb_rows) << " "
              << sums_str(mtrx_sum_cols(m, nb_rows, nb_cols), nb_cols) << " "
              << dgsttohex(mtrx_md5dgst(m, mtrx_s), MD5_DIGEST_LENGTH) << " "
              << seed
              << std::endl;
    return 0;
}