#!/bin/sh
g++ utils.cpp mtrx_funcs.cpp main.cpp -lcrypto -g -o main && ./main
