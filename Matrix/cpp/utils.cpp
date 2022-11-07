#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include "utils.h"

bool hash_eq(const unsigned char *dgst1, const unsigned char *dgst2, size_t size)
{
    for (size_t i = 0; i < size; i++)
        if (dgst1[i] != dgst2[i])
            return false;
    return true;
}
std::string int_lst_str(int *ints, size_t size)
{
    std::stringstream ss;
    ss << "[";
    for (int i = 0; i < size; i++)
    {
        if (i != 0)
        {
            ss << ",";
        }
        ss << ints[i];
    }
    ss << "]";
    return ss.str();
}

bool exists(int n, int *ints, size_t size)
{
    //std::cout << "exists req: " << n << ": " << int_lst_str(ints, size) << std::endl;
    for (size_t i = 0; i < size; i++)
        if (ints[i] == n)
            return true;
    return false;
}