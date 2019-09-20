#!/bin/bash

cat ../test.kl | valgrind --leak-check=full src/koala