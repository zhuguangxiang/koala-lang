#!/bin/bash
echo "total:"
find . -regextype posix-extended -regex ".*\.(c|h|l|y|kl)" | xargs cat | wc -l
echo "no blank:"
find . -regextype posix-extended -regex ".*\.(c|h|l|y|kl)" | xargs cat | grep -v ^$ | wc -l

cloc .