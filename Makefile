#####################################

HOSTOS := $(shell uname -s | tr '[:upper:]' '[:lower:]')

TOPDIR = $(shell pwd)

export	TOPDIR

include $(TOPDIR)/config.mk

######################################

KOALA_LIB_FILES = log.c hashtable.c hash.c vector.c buffer.c \
atom.c symbol.c object.c stringobject.c tupleobject.c tableobject.c \
moduleobject.c methodobject.c routine.c frameloop.c thread.c codeformat.c \
mod_io.c koala.c

KOALA_LIB = koala

KOALAC = koalac

KOALAC_FILES =

KOALA_FILES =

KOALA  = koala

######################################
##@$(CC) -c $(CFLAGS) $(KOALA_LIB_FILES)
##@$(AR) -rc lib$(KOALA_LIB).a $(patsubst %.c, %.o, $(KOALA_LIB_FILES))

all:

lib:
	@$(CC) -fPIC -shared $(CFLAGS) -o lib$(KOALA_LIB).so $(KOALA_LIB_FILES)
	@cp lib$(KOALA_LIB).so /usr/lib/koala-lang/
	@$(RM) *.o

parser:
	@bison -dvt -Wall -o koala_yacc.c yacc/koala.y
	@flex -o koala_lex.c yacc/koala.l
	@gcc -g -std=gnu99 -Wall koala_yacc.c koala_lex.c ast.c parser.c \
	-L. -lkoala -pthread -lrt

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
	@$(CC) $(CFLAGS) test_codeformat.c -L. -l$(KOALA_LIB) -lrt
	@./a.out

testrt: lib
	@$(CC) $(CFLAGS) test_routine.c -l$(KOALA_LIB) -L. -lrt
	@./a.out

testloop: lib
	@$(CC) $(CFLAGS) test_frameloop.c -l$(KOALA_LIB) -L. -lrt
	@./a.out
testbuf: lib
	@$(CC) $(CFLAGS) test_buffer.c -l$(KOALA_LIB) -L. -lrt
	@./a.out
test: testbuf testloop testrt testcode testlist testmodule \
testobject teststring testtuple testvector
	@echo "Test Down!"
