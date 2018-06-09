#####################################

HOST = $(shell uname)

CC = gcc

DBGFLAGS = -g -DLOG_WARN -DLOG_DEBUG #-DSHOW_ENABLED
OPTFLAGS = #-O2

CPPFLAGS = -std=gnu99 $(DBGFLAGS) $(OPTFLAGS) -I./ -Wbad-function-cast

CFLAGS = $(CPPFLAGS) -fPIC -W -Wall -Wpointer-arith -Wstrict-prototypes

YACC = bison
FLEX = flex

######################################

KOALA_OBJS = log.o hashtable.o hash.o vector.o buffer.o properties.o \
atomtable.o object.o stringobject.o tupleobject.o \
tableobject.o moduleobject.o codeobject.o opcode.o \
klc.o routine.o thread.o mod_lang.o mod_io.o kstate.o \
typedesc.o numberobject.o gc.o

KOALAC_OBJS = parser.o ast.o checker.o symbol.o codegen.o \
koala_lex.o koala_yacc.o

######################################

.PHONY: all
all: koala

libkoala.so: $(KOALA_OBJS)
	@echo "[SO]	$@"
	@$(CC) -shared -Wl,-soname,libkoala.so -o $@ $(KOALA_OBJS) -pthread
	##$(shell [ ! -f "./libkoala.so.0" ] || { ln -s libkoala.so libkoala.so.0; })
	##@cp $@ /usr/lib/koala-lang/

libkoalac.so: $(KOALAC_OBJS) libkoala.so
	@echo "[SO]	$@"
	@$(CC) -shared -Wl,-soname,libkoalac.so -o $@ $(KOALAC_OBJS) -L. -lkoala -pthread
	##$(shell [ ! -f "./libkoalac.so.0" ] || { ln -s libkoalac.so libkoalac.so.0; })
	##@cp $@ /usr/lib/koala-lang/

koala: libkoalac.so koala.o
	@echo "[MAIN]	$@"
	@gcc $(CFLAGS) -o $@ koala.o -L. -lkoalac -lkoala -pthread -lrt
	##@cp $@ /usr/local/bin

koala_yacc.c: yacc/koala.y
	@echo "[YACC]	$@"
	@$(YACC) -dvt -Wall -o $@ yacc/koala.y

koala_lex.c: yacc/koala.l
	@echo "[FLEX]	$@"
	@$(FLEX) -o $@ yacc/koala.l

.PHONY: clean
clean:
	@rm -f *.so *.o *.d *.d.* koala_lex.* koala_yacc.*

######################################

ifneq ($(MAKECMDGOALS), clean)
sinclude $(KOALA_OBJS:.o=.d) $(KOALAC_OBJS:.o=.d) koala.d koalac.d
endif

%.o: %.c
	@echo "[CC]	$@"
	@$(CC) $(CFLAGS) -c -o $@ $<

%.d: %.c
	@set -e; rm -f $@; \
	$(CC) -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

######################################

testvector:
	@$(CC) $(CFLAGS) test_vector.c -l$(KOALA_LIB) -L. -lrt
	@./a.out

testtuple:
	@$(CC) $(CFLAGS) test_tuple.c -l$(KOALA_LIB) -L. -lrt
	@./a.out

testthread:
	@$(CC) $(CFLAGS) test_thread.c -l$(KOALA_LIB) -L. -lrt
	@./a.out

teststring:
	@$(CC) $(CFLAGS) test_string.c -l$(KOALA_LIB) -L. -lrt
	@./a.out

testobject:
	@$(CC) $(CFLAGS) test_object.c -l$(KOALA_LIB) -L. -lrt
	@./a.out

testmodule:
	@$(CC) $(CFLAGS) test_module.c -l$(KOALA_LIB) -L. -lrt
	@./a.out

testlist:
	@$(CC) $(CFLAGS) test_list.c -l$(KOALA_LIB) -L. -lrt
	@./a.out

testhashtable:
	@$(CC) $(CFLAGS) test_hashtable.c -l$(KOALA_LIB) -L. -lrt
	@./a.out

testimage:
	@$(CC) $(CFLAGS) test_image.c -L. -l$(KOALA_LIB) -lrt
	@./a.out

testroutine:
	@$(CC) $(CFLAGS) test_routine.c -l$(KOALA_LIB) -L. -lrt
	@./a.out

testloop:
	@$(CC) $(CFLAGS) test_loop.c -l$(KOALA_LIB) -L. -lrt
	@./a.out

testbuf:
	@$(CC) $(CFLAGS) test_buffer.c -l$(KOALA_LIB) -L. -lrt
	@./a.out

testprop:
	@$(CC) $(CFLAGS) test_properties.c -l$(KOALA_LIB) -L. -lrt
	@./a.out

testnumber:
	@$(CC) $(CFLAGS) test_number.c -l$(KOALA_LIB) -L. -lrt
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
	@cd test/ && mv town.klc ~/.koala-repo/test
	@cd test/ && $(KOALAC) person.kl
	@cd test/ && mv person.klc ~/.koala-repo/test
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

test-0.5.11:
	@cd test/ && $(KOALAC) animal.kl
	@cd test/ && mv animal.klc ~/.koala-repo/test
	@$(RM) test-0.5.11.klc
	@$(KOALAC) test-0.5.11.kl
	@$(KOALA) test-0.5.11

test-0.5.12:
	@$(RM) test-0.5.12.klc
	@$(KOALAC) test-0.5.12.kl
	@$(KOALA) test-0.5.12

test-0.5.13:
	@$(RM) test-0.5.13.klc
	@$(KOALAC) test-0.5.13.kl
	@$(KOALA) test-0.5.13

test-0.5.14:
	@$(RM) test-0.5.14.klc
	@$(KOALAC) test-0.5.14.kl
	@$(KOALA) test-0.5.14

test-number:
	@$(RM) test-number.klc
	@$(KOALAC) test-number.kl
	@$(KOALA) test-number

test-trait-0:
	@$(RM) test-trait-0.klc
	@$(KOALAC) test-trait-0.kl
	@$(KOALA) test-trait-0

test-trait-1:
	@$(RM) test-trait-1.klc
	@$(KOALAC) test-trait-1.kl
	@$(KOALA) test-trait-1

test-trait-2:
	@$(RM) test-trait-2.klc
	@$(KOALAC) test-trait-2.kl
	@$(KOALA) test-trait-2

test-trait-3:
	@$(RM) test-trait-3.klc
	@$(KOALAC) test-trait-3.kl
	@$(KOALA) test-trait-3

test-trait-4:
	@$(RM) test-trait-4.klc
	@$(KOALAC) test-trait-4.kl
	@$(KOALA) test-trait-4

test-trait-5:
	@$(RM) test-trait-5.klc
	@$(KOALAC) test-trait-5.kl
	@$(KOALA) test-trait-5

testkl: test-0.5.1 test-0.5.2 test-0.5.3 test-0.5.4 test-0.5.5 test-0.5.6 \
	test-0.5.7 test-0.5.8 test-test test-0.5.9 test-0.5.10 test-0.5.11 \
	test-0.5.12 test-0.5.13 test-0.5.14 test-number
	@echo "Test Koala Down!"

testtrait: test-trait-0 test-trait-1 test-trait-2 test-trait-3 \
	test-trait-4 test-trait-5
	@echo "Test Koala Down!"

runkl:
	@$(KOALA) test-0.5.1
	@$(KOALA) test-0.5.2
	@$(KOALA) test-0.5.3
	@$(KOALA) test-0.5.4
	@$(KOALA) test-0.5.5
	@$(KOALA) test-0.5.6
	@$(KOALA) test-0.5.7
	@$(KOALA) test-0.5.8
	@cd test/ && $(KOALA) test
	@$(KOALA) test-0.5.9
	@$(KOALA) test-0.5.10
	@$(KOALA) test-0.5.11
	@$(KOALA) test-0.5.12
	@$(KOALA) test-0.5.13
	@$(KOALA) test-0.5.14
	@$(KOALA) test-0.5.15
	@$(KOALA) test-number

runtrait:
	@$(KOALA) test-trait-0
	@$(KOALA) test-trait-1
	@$(KOALA) test-trait-2
	@$(KOALA) test-trait-3
	@$(KOALA) test-trait-4
	@$(KOALA) test-trait-5

test: testprop testbuf testroutine testimage testhashtable testlist \
	testmodule testobject teststring testtuple testvector testkl testtrait
	@echo "Test Down!"
