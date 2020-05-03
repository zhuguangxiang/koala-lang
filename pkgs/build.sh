#!/bin/bash

modules=(fmt)

for m in $modules
do
  echo "compile '$m' module"
  koala -c $m
done

if [ -n "$KOALA_HOME" ]; then
  installed=$KOALA_HOME/pkgs
  if [ ! -d $installed ]; then
    mkdir $installed
  fi
  echo "copy '*.klc' into '$installed'"
  find . -name "*.klc" | xargs cp --parents -t $installed
fi

echo "pack '*.kl' into src.izp"
find . -name "*.kl" | xargs zip src.zip
