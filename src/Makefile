
#############################################################

# MPILEAKS_LIB, MPILEAKS_LIBDIR
include ../defs.mk

TOOLS = /usr/global/tools
# Mpileaks dependencies:
CALLPATH = $(TOOLS)/mpileaks
CHAOS4IB = $(TOOLS)/adept-utils/chaos_4_x86_64_ib
STACKWALKER = $(TOOLS)/dyninst/stackwalker/chaos_4_x86_64_ib/stackwalker-2.0


#############################################################

# If variable is defined, include persistent communication
# whose container is a stack of callpaths. Otherwise, a simpler 
# version with callpaths is used. 
PERSISTENT_COMM = 1

CC = mpig++
OPT = -Wall -Werror
LEAKS_FLAGS = -g -O0 -fPIC

LEAKS_INCLUDES = -I$(CALLPATH)/include 
LEAKS_INCLUDES += -I$(CHAOS4IB)/include

TESTS = 
LEAKS_OBJS = mpileaks.o fileio.o datatype.o
ifdef PERSISTENT_COMM
LEAKS_OBJS += sendrecv_persistent.o
else
LEAKS_OBJS += sendrecv.o
endif 


all: $(MPILEAKS_LIB) $(TESTS)


mpileaks.o: mpileaks.cpp 
	$(CC) $(OPT) $(LEAKS_FLAGS) $(LEAKS_INCLUDES) -c -o $@ mpileaks.cpp

sendrecv.o: sendrecv.cpp mpileaks.h
	$(CC) $(OPT) $(LEAKS_FLAGS) $(LEAKS_INCLUDES) -c -o $@ sendrecv.cpp

fileio.o: fileio.cpp mpileaks.h
	$(CC) $(OPT) $(LEAKS_FLAGS) $(LEAKS_INCLUDES) -c -o $@ fileio.cpp

sendrecv_persistent.o: sendrecv_persistent.cpp mpileaks.h
	$(CC) $(OPT) $(LEAKS_FLAGS) $(LEAKS_INCLUDES) -c -o $@ sendrecv_persistent.cpp

datatype.o: datatype.cpp mpileaks.h
	$(CC) $(OPT) $(LEAKS_FLAGS) $(LEAKS_INCLUDES) -c -o $@ datatype.cpp

$(MPILEAKS_LIB): $(LEAKS_OBJS)
	$(CC) $(LEAKS_FLAGS) -shared -Wl,-soname,$(MPILEAKS_LIB).1 -o $@ $(LEAKS_OBJS) \
	-Wl,-rpath,$(CALLPATH)/lib -L$(CALLPATH)/lib -lcallpath \
	-Wl,-rpath,$(STACKWALKER)/lib -L$(STACKWALKER)/lib -lcommon \
	-Wl,-rpath,$(CHAOS4IB)/lib -L$(CHAOS4IB)/lib
	if [ ! -d "$(MPILEAKS_LIBDIR)" ]; then mkdir -p "$(MPILEAKS_LIBDIR)"; fi 
	cp $@ $(MPILEAKS_LIBDIR)/
	cd $(MPILEAKS_LIBDIR); chown $(USER):tools $@; reln -g tools $@ $@.1

clean: 
	rm -rf *~ *.o $(MPILEAKS_LIB)* $(TESTS)

veryclean: clean
	rm -rf $(MPILEAKS_LIBDIR)/$(MPILEAKS_LIB)*
