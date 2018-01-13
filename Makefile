#####################################

HOSTOS := $(shell uname -s | tr '[:upper:]' '[:lower:]')

TOPDIR = $(shell pwd)

export	TOPDIR

include $(TOPDIR)/config.mk

######################################

KOALA_LIB_FILES = hashtable.c hash.c vector.c itemtable.c symbol.c \
object.c stringobject.c tupleobject.c tableobject.c moduleobject.c \
methodobject.c codeformat.c debug.c routine.c thread.c #kstate.c koala.c
##coroutine.c

KOALA_LIB = koala

KOALAC = koalac

KOALAC_FILES =

KOALA_FILES =

KOALA  = koala

######################################

all:

lib:
	@$(RM) lib*.a
	@$(CC) -c $(CFLAGS) $(KOALA_LIB_FILES)
	@$(AR) -r lib$(KOALA_LIB).a $(patsubst %.c, %.o, $(KOALA_LIB_FILES))
	@$(RM) *.o

testmodule: lib
	@$(CC) $(CFLAGS) test_module.c -l$(KOALA_LIB) -L.
format: lib
	@$(CC) $(CFLAGS) test_codeformat.c -l$(KOALA_LIB) -L.
