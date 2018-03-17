#####################################

HOSTOS := $(shell uname -s | tr '[:upper:]' '[:lower:]')

TOPDIR = $(shell pwd)

export	TOPDIR

include $(TOPDIR)/config.mk

######################################

KOALA_LIB_FILES = log.c hashtable.c hash.c vector.c buffer.c properties.c \
atomtable.c symbol.c object.c stringobject.c tupleobject.c \
tableobject.c moduleobject.c classobject.c codeobject.c opcode.c codegen.c \
klc.c routine.c thread.c mod_io.c koala_state.c \
koala_yacc.c koala_lex.c ast.c parser.c checker.c koala_type.c
KOALA_LIB = koala

KOALAC_FILES =  koalac.c
KOALAC = koalac

KOALA_FILES = koala.c
KOALA  = koala

######################################
##@$(CC) -c $(CFLAGS) $(KOALA_LIB_FILES)
##@$(AR) -rc lib$(KOALA_LIB).a $(patsubst %.c, %.o, $(KOALA_LIB_FILES))

all: lib koalac koala test

lib:
	@bison -dvt -Wall -o koala_yacc.c yacc/koala.y
	@flex -o koala_lex.c yacc/koala.l
	@$(CC) -fPIC -shared $(CFLAGS) -o lib$(KOALA_LIB).so $(KOALA_LIB_FILES) \
	-pthread
	@cp lib$(KOALA_LIB).so /usr/lib/koala-lang/
	@$(RM) *.o lib$(KOALA_LIB).so
	@$(RM) koala_yacc.h koala_yacc.c koala_yacc.output koala_lex.c

koalac:
	@gcc $(CFLAGS) -o $(KOALAC) $(KOALAC_FILES) -L. -l$(KOALA_LIB) -pthread -lrt
	@cp $(KOALAC) /usr/local/bin
	@$(RM) $(KOALAC)

koala:
	@gcc $(CFLAGS) -o $(KOALA) $(KOALA_FILES) -L. -l$(KOALA_LIB) -pthread -lrt
	@cp $(KOALA) /usr/local/bin
	@$(RM) $(KOALA)

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
	@$(CC) $(CFLAGS) test_image.c -L. -l$(KOALA_LIB) -lrt
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

testprop: lib
	@$(CC) $(CFLAGS) test_properties.c -l$(KOALA_LIB) -L. -lrt
	@./a.out

test-0.5.1:
	@$(RM) test-0.5.1.klc
	@$(KOALAC) test-0.5.1.kl
	@$(KOALA) test-0.5.1

test-0.5.2:
	@$(RM) test-0.5.2.klc
	@$(KOALAC) test-0.5.2.kl
	@$(KOALA) test-0.5.2

test-0.5.3:
	@$(RM) test-0.5.3.klc
	@$(KOALAC) test-0.5.3.kl
	@$(KOALA) test-0.5.3

test-0.5.4:
	@$(RM) test-0.5.4.klc
	@$(KOALAC) test-0.5.4.kl
	@$(KOALA) test-0.5.4

test-0.5.5:
	@$(RM) test-0.5.5.klc
	@$(KOALAC) test-0.5.5.kl
	@$(KOALA) test-0.5.5

test-0.5.6:
	@$(RM) test-0.5.6.klc
	@$(KOALAC) test-0.5.6.kl
	@$(KOALA) test-0.5.6

test-0.5.7:
	@$(RM) test-0.5.7.klc
	@$(KOALAC) test-0.5.7.kl
	@$(KOALA) test-0.5.7

test-0.5.8:
	@$(RM) test-0.5.8.klc
	@$(KOALAC) test-0.5.8.kl
	@$(KOALA) test-0.5.8

test-test:
	@cd test/ && $(KOALAC) town.kl
	@cd test/ && mv town.klc ~/koala-repo/test
	@cd test/ && $(KOALAC) person.kl
	@cd test/ && mv person.klc ~/koala-repo/test
	@cd test/ && $(RM) test.klc
	@cd test/ && $(KOALAC) test.kl
	@cd test/ && $(KOALA) test

test-0.5.9:
	@$(RM) test-0.5.9.klc
	@$(KOALAC) test-0.5.9.kl
	@$(KOALA) test-0.5.9

test-0.5.10:
	@$(RM) test-0.5.10.klc
	@$(KOALAC) test-0.5.10.kl
	@$(KOALA) test-0.5.10

testkl: test-0.5.1 test-0.5.2 test-0.5.3 test-0.5.4 test-0.5.5 test-0.5.6 \
	test-0.5.7 test-0.5.8 test-test test-0.5.9 test-0.5.10
	@echo "Test Koala Down!"

test: testprop testbuf testroutine testimage testhashtable testlist \
	testmodule testobject teststring testtuple testvector testkl
	@echo "Test Down!"

.PHONY: all
.PHONY: koala
.PHONY: koalac
.PHONY: lib
