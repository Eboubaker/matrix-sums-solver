#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include "utils.h"

#ifdef _WIN32
#include <windows.h>
#elif MACOS
#include <sys/param.h>
#include <sys/sysctl.h>
#else
#include <unistd.h>
#endif

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

int get_num_cores()
{
#ifdef WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
#elif MACOS
    int nm[2];
    size_t len = 4;
    uint32_t count;

    nm[0] = CTL_HW;
    nm[1] = HW_AVAILCPU;
    sysctl(nm, 2, &count, &len, NULL, 0);

    if (count < 1)
    {
        nm[1] = HW_NCPU;
        sysctl(nm, 2, &count, &len, NULL, 0);
        if (count < 1)
        {
            count = 1;
        }
    }
    return count;
#else
    return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}