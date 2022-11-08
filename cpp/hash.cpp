#include "hash.h"

int main(int argc, char** argv){
    auto arg1 = std::string(argv[1]);
    arg1 = arg1.substr(1, arg1.size() - 2);
    auto splits = split(arg1, ",");
    int size = splits.size();
    t_cell m[size];
    for (int i = 0; i < size; i++)
    {
        m[i] = std::stoi(splits[i]);
    }
    std::cout << dgsttohex(mtrx_md5dgst(m, size), MD5_DIGEST_LENGTH) << std::endl;
    return 0;
}