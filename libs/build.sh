#!/bin/bash

koalac --build-stdlib --cgen --write-klc --fusion std/builtin --package-name=std/builtin

koala -c std/io --package-name=std/io
koala -c std/fs --package-name=std/fs
koala -c std/os --package-name=std/os
koala -c std/sys --package-name=std/sys
koala -c std/time --package-name=std/time
koala -c std/ut.kl --package-name=std/ut

koala -c koala/pretty.kl --package-name=koala/pretty
