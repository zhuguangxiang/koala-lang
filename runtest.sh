#!/bin/bash

cat ./test.kl | valgrind --leak-check=full koala
cat ./test_match.kl | valgrind --leak-check=full koala
cat ./test_testblock.kl | valgrind --leak-check=full koala
