#!/bin/bash
echo "total:"
find . -name "*.[ch]" | xargs cat | wc -l
echo "no blank:"
find . -name "*.[ch]" | xargs cat | grep -v ^$ | wc -l
