#!/bin/bash

modules=(io fmt sys lang)

for m in $(modules[@])
do
  echo "compile '$m' module"
  koala -c $m
done

#installed=$KOALA_HOME/pkg
#if [ ! -d $installed ]; then
#  mkdir $installed
#if

# find . -name ".klc" | xargs cp --parents -t $installed
find . -name ".kl" | xargs zip src.zip
