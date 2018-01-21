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
	@$(AR) -rc lib$(KOALA_LIB).a $(patsubst %.c, %.o, $(KOALA_LIB_FILES))
	@$(RM) *.o

testvector: lib
	@$(CC) $(CFLAGS) test_vector.c -l$(KOALA_LIB) -L. -lrt
	@./a.out
testtuple: lib
	@$(CC) $(CFLAGS) test_tuple.c -l$(KOALA_LIB) -L. -lrt
	@./a.out
testthread: lib
	@$(CC) $(CFLAGS) test_thread.c -l$(KOALA_LIB) -L. -lrt
	@./a.out
teststring: lib
	@$(CC) $(CFLAGS) test_string.c -l$(KOALA_LIB) -L. -lrt
	@./a.out
testobject: lib
	@$(CC) $(CFLAGS) test_object.c -l$(KOALA_LIB) -L. -lrt
	@./a.out
testmodule: lib
	@$(CC) $(CFLAGS) test_module.c -l$(KOALA_LIB) -L. -lrt
	@./a.out
testlist: lib
	@$(CC) $(CFLAGS) test_list.c -l$(KOALA_LIB) -L. -lrt
	@./a.out
testhashtable: lib
	@$(CC) $(CFLAGS) test_hashtable.c -l$(KOALA_LIB) -L. -lrt
	@./a.out
testcode: lib
	@$(CC) $(CFLAGS) test_codeformat.c -l$(KOALA_LIB) -L. -lrt
	@./a.out
testrt: lib
	@$(CC) $(CFLAGS) test_routine.c -l$(KOALA_LIB) -L. -lrt
	@./a.out
testloop: lib
	@$(CC) $(CFLAGS) test_frameloop.c -l$(KOALA_LIB) -L. -lrt
	@./a.out
