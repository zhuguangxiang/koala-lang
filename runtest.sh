#!/bin/bash

cat ../test.kl | valgrind --leak-check=full src/koala
cat ../test_match.kl | valgrind --leak-check=full src/koala