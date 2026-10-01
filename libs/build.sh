#!/bin/bash

python3 build.py --src ./std

koala -c koala/pretty.kl --package-name=koala/pretty
