# Koala

## opcode

opcode with type

## stack

use continue stack and load/store

## gc

use semi-copy gc

## object layout

+---------------+
| virtual table |
+---------------+
| data/pointer  |
+---------------+

## collection

slice will not save virtual table, only save data/pointer

+-----------------+
| length          |
+-----------------+
| data/pointer/0  |
+-----------------+
| data/pointer/1  |
+-----------------+
| data/pointer/2  |
+-----------------+
| data/pointer/3  |
+-----------------+

## value object

int, float, bool, char

## reference object

string, slice, map
