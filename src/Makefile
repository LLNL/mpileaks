
#############################################################

# MPILEAKS_LIB, MPILEAKS_LIBDIR
include ../defs.mk

TOOLS = /usr/global/tools
# Mpileaks dependencies:
#CALLPATH = $(TOOLS)/mpileaks
CALLPATH = $(TOOLS)/mpileaks/mvapich-gnu
#CALLPATH = $(TOOLS)/mpileaks/mvapich-intel
#CALLPATH = $(TOOLS)/mpileaks/mvapich2-gnu
CHAOS4IB = $(TOOLS)/adept-utils/chaos_4_x86_64_ib
STACKWALKER = $(TOOLS)/dyninst/stackwalker/chaos_4_x86_64_ib/stackwalker-2.0


#############################################################

# If variable is defined, include persistent communication
# whose container is a stack of callpaths. Otherwise, a simpler 
# version with callpaths is used. 
PERSISTENT_COMM = 1

CC = mpig++
#CC = mpiicpc
#CC = mpicxx
OPT = -Wall -Werror
LEAKS_FLAGS = -g -O0 -fPIC

LEAKS_INCLUDES = -I$(CALLPATH)/include 
LEAKS_INCLUDES += -I$(CHAOS4IB)/include

TESTS = 
LEAKS_OBJS = mpileaks.o comm.o datatype.o errhandler.o fileio.o group.o info.o keyval.o mem.o op.o request.o win.o


all: $(MPILEAKS_LIB) $(TESTS)


mpileaks.o: mpileaks.cpp callpath2count.h 
	$(CC) $(OPT) $(LEAKS_FLAGS) $(LEAKS_INCLUDES) -c -o $@ mpileaks.cpp

comm.o: comm.cpp mpileaks.h callpath2count.h 
	$(CC) $(OPT) $(LEAKS_FLAGS) $(LEAKS_INCLUDES) -c -o $@ comm.cpp

datatype.o: datatype.cpp mpileaks.h callpath2count.h 
	$(CC) $(OPT) $(LEAKS_FLAGS) $(LEAKS_INCLUDES) -c -o $@ datatype.cpp

errhandler.o: errhandler.cpp mpileaks.h callpath2count.h 
	$(CC) $(OPT) $(LEAKS_FLAGS) $(LEAKS_INCLUDES) -c -o $@ errhandler.cpp

fileio.o: fileio.cpp mpileaks.h callpath2count.h 
	$(CC) $(OPT) $(LEAKS_FLAGS) $(LEAKS_INCLUDES) -c -o $@ fileio.cpp

group.o: group.cpp mpileaks.h callpath2count.h 
	$(CC) $(OPT) $(LEAKS_FLAGS) $(LEAKS_INCLUDES) -c -o $@ group.cpp

info.o: info.cpp mpileaks.h callpath2count.h 
	$(CC) $(OPT) $(LEAKS_FLAGS) $(LEAKS_INCLUDES) -c -o $@ info.cpp

keyval.o: keyval.cpp mpileaks.h callpath2count.h 
	$(CC) $(OPT) $(LEAKS_FLAGS) $(LEAKS_INCLUDES) -c -o $@ keyval.cpp

mem.o: mem.cpp mpileaks.h callpath2count.h 
	$(CC) $(OPT) $(LEAKS_FLAGS) $(LEAKS_INCLUDES) -c -o $@ mem.cpp

op.o: op.cpp mpileaks.h callpath2count.h 
	$(CC) $(OPT) $(LEAKS_FLAGS) $(LEAKS_INCLUDES) -c -o $@ op.cpp

request.o: request.cpp mpileaks.h callpath2count.h 
	$(CC) $(OPT) $(LEAKS_FLAGS) $(LEAKS_INCLUDES) -c -o $@ request.cpp

win.o: win.cpp mpileaks.h callpath2count.h 
	$(CC) $(OPT) $(LEAKS_FLAGS) $(LEAKS_INCLUDES) -c -o $@ win.cpp


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
