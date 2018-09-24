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
atomtable.o object.o stringobject.o tupleobject.o listobject.o\
tableobject.o moduleobject.o codeobject.o opcode.o \
klc.o routine.o thread.o mod_lang.o mod_io.o koalastate.o \
typedesc.o numberobject.o gc.o options.o

KOALAC_OBJS = parser.o ast.o checker.o symbol.o codegen.o \
koala_lex.o koala_yacc.o

######################################

.PHONY: all
all: koala koalac
	@cp koala ~/.local/bin
	@cp koalac ~/.local/bin
	@cp libkoala.so ~/.local/lib
	@cp libkoalac.so ~/.local/lib

libkoala.so: $(KOALA_OBJS)
	@echo "	[SO]	$@"
	@$(CC) -fPIC -shared $(CFLAGS) -o $@ $^ -pthread

libkoalac.so: $(KOALAC_OBJS)
	@echo "	[SO]	$@"
	@$(CC) -fPIC -shared $(CFLAGS) -o $@ $^ -pthread

koala: koala.o libkoala.so
	@echo "	[CC]	$@"
	@$(CC) $(CFLAGS) -o $@ $< -L. -lkoala -pthread -lrt

koalac: koalac.o libkoalac.so libkoala.so
	@echo "	[CC]	$@"
	@$(CC) $(CFLAGS) -o $@ $< -L. -lkoalac -lkoala -pthread -lrt

koala_yacc.c: yacc/koala.y
	@echo "	[YACC]	$@"
	@$(YACC) -dvt -Wall -o $@ $^

koala_lex.c: yacc/koala.l
	@echo "	[FLEX]	$@"
	@$(FLEX) -o $@ $^

.PHONY: clean
clean:
	@rm -f *.so *.o *.d *.d.* koalac koala koala_lex.* koala_yacc.*

######################################

ifneq ($(MAKECMDGOALS), clean)
sinclude $(KOALA_OBJS:.o=.d) $(KOALAC_OBJS:.o=.d) koala.d
endif

%.o: %.c
	@echo "	[CC]	$@"
	@$(CC) $(CFLAGS) -c -o $@ $<

%.d: %.c
	@set -e; rm -f $@; \
	$(CC) -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

######################################
