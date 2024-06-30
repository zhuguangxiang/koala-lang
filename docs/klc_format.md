
# Koala Code File Format

One file which suffix is `.klc` is a `Koala` code file. One klc file represents a module.
The dot klc file is Stream Object Format(`SOF`). It is divided into three parts.
The first part is header part, which includes magic number and version information
for validating the dot klc file is valid. The second part is meta info part, which includes
global variables, functions and class(trait) information. The last one part is code part,
which is code objects that are streamed one by one.

## Basic

There are some basic objects needed to explain firstly.
These basic objects are int, float, string, array etc. They are not the language's
concepts, but are `SOF` concepts.



## Header

## Meta

## Code
