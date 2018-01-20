#####################################

HOSTOS := $(shell uname -s | tr '[:upper:]' '[:lower:]')

TOPDIR = $(shell pwd)

export	TOPDIR

include $(TOPDIR)/config.mk

######################################

KOALA_LIB_FILES = debug.c hashtable.c hash.c vector.c itemtable.c symbol.c \
object.c stringobject.c tupleobject.c tableobject.c moduleobject.c \
methodobject.c routine.c frameloop.c thread.c codeformat.c \
mod_io.c koala.c

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

testvector: lib
	@$(CC) $(CFLAGS) test_vector.c -l$(KOALA_LIB) -L. -lrt
testtuple: lib
	@$(CC) $(CFLAGS) test_tuple.c -l$(KOALA_LIB) -L. -lrt
testthread: lib
	@$(CC) $(CFLAGS) test_thread.c -l$(KOALA_LIB) -L. -lrt
teststring: lib
	@$(CC) $(CFLAGS) test_string.c -l$(KOALA_LIB) -L. -lrt
testobject: lib
	@$(CC) $(CFLAGS) test_object.c -l$(KOALA_LIB) -L. -lrt
testmodule: lib
	@$(CC) $(CFLAGS) test_module.c -l$(KOALA_LIB) -L. -lrt
testlist: lib
	@$(CC) $(CFLAGS) test_list.c -l$(KOALA_LIB) -L. -lrt
testhashtable: lib
	@$(CC) $(CFLAGS) test_hashtable.c -l$(KOALA_LIB) -L. -lrt
testcode: lib
	@$(CC) $(CFLAGS) test_codeformat.c -l$(KOALA_LIB) -L. -lrt
testrt: lib
	@$(CC) $(CFLAGS) test_routine.c -l$(KOALA_LIB) -L. -lrt
testloop: lib
	@$(CC) $(CFLAGS) test_frameloop.c -l$(KOALA_LIB) -L. -lrt
