
#include <stdlib.h>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <vector>
#include <string.h>
#include <assert.h>
#include <chrono>

int get_num_cores();
bool hash_eq(const unsigned char *dgst1, const unsigned char *dgst2, size_t size);
std::string dgsttohex(const unsigned char *dgst, size_t size);
void hextodgst(const char *src, unsigned char *target);
bool exists(int n, int *ints, size_t size);
std::string int_lst_str(int *ints, size_t size);
std::vector<std::string> split(std::string s, std::string delimiter);
uint32_t millis();