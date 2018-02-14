
############################
#
# 交叉工具相关设置
#
############################

CROSS_COMPILE =

AS	= $(CROSS_COMPILE)as
LD	= $(CROSS_COMPILE)ld
CC	= $(CROSS_COMPILE)gcc
CPP	= $(CC) -E
AR	= $(CROSS_COMPILE)ar
NM	= $(CROSS_COMPILE)nm
STRIP	= $(CROSS_COMPILE)strip
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump

############################
#
# 编译器相关标志设置
#
############################

DBGFLAGS = -g -DLOG_WARN -DLOG_NDEBUG
OPTFLAGS = #-O2

CPPFLAGS = -std=gnu99 $(DBGFLAGS) $(OPTFLAGS) -I$(TOPDIR) -Wbad-function-cast

CFLAGS = $(CPPFLAGS) -W -Wall -Wpointer-arith -Wstrict-prototypes -pthread

#########################################################################

export	CROSS_COMPILE AS LD CC CPP AR NM STRIP OBJCOPY OBJDUMP MAKE

export	CPPFLAGS CFLAGS

#########################################################################

############################
#
# $(CURDIR)由make自动产生, make -C
#
############################

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
