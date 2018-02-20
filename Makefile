#####################################

HOSTOS := $(shell uname -s | tr '[:upper:]' '[:lower:]')

TOPDIR = $(shell pwd)

export	TOPDIR

include $(TOPDIR)/config.mk

######################################

KOALA_LIB_FILES = log.c hashtable.c hash.c vector.c buffer.c \
atomtable.c codeblock.c symbol.c object.c stringobject.c tupleobject.c \
tableobject.c moduleobject.c codeobject.c codeimage.c opcode.c \
routine.c thread.c mod_io.c koalastate.c
KOALA_LIB = koala

KOALAC_FILES = koala_yacc.c koala_lex.c ast.c parser.c
KOALAC = koalac

KOALA_FILES = koala.c
KOALA  = koala

######################################
##@$(CC) -c $(CFLAGS) $(KOALA_LIB_FILES)
##@$(AR) -rc lib$(KOALA_LIB).a $(patsubst %.c, %.o, $(KOALA_LIB_FILES))

all: lib koalac koala test

lib:
	@$(CC) -fPIC -shared $(CFLAGS) -o lib$(KOALA_LIB).so $(KOALA_LIB_FILES) \
	-pthread
	@cp lib$(KOALA_LIB).so /usr/lib/koala-lang/
	@$(RM) *.o
	@$(RM) lib$(KOALA_LIB).so

koalac:
	@bison -dvt -Wall -o koala_yacc.c yacc/koala.y
	@flex -o koala_lex.c yacc/koala.l
	@gcc $(CFLAGS) -o $(KOALAC) $(KOALAC_FILES) \
	-L. -l$(KOALA_LIB) -pthread -lrt
	@cp $(KOALAC) /usr/local/bin
	@rm koala_yacc.c koala_lex.c $(KOALAC)

koala:
	@gcc $(CFLAGS) -o $(KOALA) $(KOALA_FILES) -L. -l$(KOALA_LIB) -pthread -lrt
	@cp $(KOALA) /usr/local/bin
	@rm $(KOALA)

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

testimage: lib
	@$(CC) $(CFLAGS) test_codeimage.c -L. -l$(KOALA_LIB) -lrt
	@./a.out

testroutine: lib
	@$(CC) $(CFLAGS) test_routine.c -l$(KOALA_LIB) -L. -lrt
	@./a.out

testloop: lib
	@$(CC) $(CFLAGS) test_loop.c -l$(KOALA_LIB) -L. -lrt
	@./a.out

testbuf: lib
	@$(CC) $(CFLAGS) test_buffer.c -l$(KOALA_LIB) -L. -lrt
	@./a.out

test-0.5.1:
	@$(RM) test-0.5.1.klc
	@$(KOALAC) test-0.5.1.kl
	@$(KOALA) test-0.5.1.klc

test-0.5.2:
	@$(RM) test-0.5.2.klc
	@$(KOALAC) test-0.5.2.kl
	@$(KOALA) test-0.5.2.klc

test-0.5.3:
	@$(RM) test-0.5.3.klc
	@$(KOALAC) test-0.5.3.kl
	@$(KOALA) test-0.5.3.klc

test: testbuf testloop testroutine testimage testhashtable testlist \
	testmodule testobject teststring testtuple testvector \
	test-0.5.1 test-0.5.2 test-0.5.3
	@echo "Test Down!"

.PHONY: test
.PHONY: lib
.PHONY: koalac
.PHONY: koala
.PHONY: all
