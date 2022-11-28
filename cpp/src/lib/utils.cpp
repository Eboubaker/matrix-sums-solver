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
std::string int_lst_str(int *ints, int count)
{
    std::stringstream ss;
    ss << "[";
    for (int i = 0; i < count; i++)
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

// logical cores
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

std::string dgsttohex(const unsigned char *dgst, size_t size){
    char hexstr[size*size];
    size_t i;
    for (i = 0; i < size; i++)
    {
        sprintf(hexstr + i * 2, "%02x", dgst[i]);
    }
    hexstr[i * 2] = 0;
    return std::string(hexstr);
}
unsigned char char2int(char input)
{
    if (input >= '0' && input <= '9')
        return input - '0';
    if (input >= 'A' && input <= 'F')
        return input - 'A' + 10;
    if (input >= 'a' && input <= 'f')
        return input - 'a' + 10;
    throw std::invalid_argument("Invalid input string");
}

void hextodgst(const char *src, unsigned char *target)
{
    while (*src && src[1])
    {
        *(target++) = char2int(*src) * 16 + char2int(src[1]);
        src += 2;
    }
}

std::vector<std::string> split(std::string s, std::string delimiter)
{
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos)
    {
        token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back(token);
    }

    res.push_back(s.substr(pos_start));
    return res;
}

uint32_t millis()
{
    using namespace std::chrono;
    return static_cast<uint32_t>(duration_cast<milliseconds>(
                                     system_clock::now().time_since_epoch())
                                     .count());
}

int idxtorow(int index, int nb_cols){
    return index / nb_cols;
}
int idxtocol(int index, int nb_cols)
{
    return index % nb_cols;
}

std::vector<char> string_to_char(const std::vector<std::string> &strings)
{
    std::vector<char> cstrings;
    cstrings.reserve(strings.size());
    for (std::string s : strings)
    {
        for (size_t i = 0; i < strlen(s.c_str()); ++i)
        {
            cstrings.push_back(s.c_str()[i]);
        }
    }

    return cstrings;
}