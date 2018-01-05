#!/bin/bash

gcc -g -std=c99 -W -Wall codeformattest.c codeformat.c vector.c \
hash.c hashtable.c itemtable.c debug.c
