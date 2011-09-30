
CC = mpig++
OPT = -Wall -Werror
LEAKS_FLAGS = -g -O0 -fPIC

TOOLS = /usr/global/tools
LEAKS_AUX = $(TOOLS)/mpileaks
LEAKS_INCLUDES = -I$(LEAKS_AUX)/include 
LEAKS_INCLUDES += -I$(TOOLS)/adept-utils/chaos_4_x86_64_ib/include
LEAKS_LIBDIR = $(HOME)/local-mpileaks/lib

LEAKS_OBJS = mpileaks.o sendrecv.o fileio.o
LEAKS_LIB = libmpileaks.so

all: $(LEAKS_LIB) 

mpileaks.o: mpileaks.cpp 
	$(CC) $(OPT) $(LEAKS_FLAGS) $(LEAKS_INCLUDES) -c -o $@ mpileaks.cpp

sendrecv.o: sendrecv.cpp mpileaks.h
	$(CC) $(OPT) $(LEAKS_FLAGS) $(LEAKS_INCLUDES) -c -o $@ sendrecv.cpp

fileio.o: fileio.cpp mpileaks.h
	$(CC) $(OPT) $(LEAKS_FLAGS) $(LEAKS_INCLUDES) -c -o $@ fileio.cpp

$(LEAKS_LIB): $(LEAKS_OBJS)
	$(CC) $(LEAKS_FLAGS) -shared -Wl,-soname,$(LEAKS_LIB).1 -o $@ $(LEAKS_OBJS) \
	-Wl,-rpath,$(LEAKS_AUX)/lib -L$(LEAKS_AUX)/lib -lcallpath \
	-Wl,-rpath,$(TOOLS)/adept-utils/chaos_4_x86_64_ib/lib \
	-L$(TOOLS)/adept-utils/chaos_4_x86_64_ib/lib \
	-Wl,-rpath,$(TOOLS)/dyninst/stackwalker/chaos_4_x86_64_ib/stackwalker-2.0/lib \
	-L$(TOOLS)/dyninst/stackwalker/chaos_4_x86_64_ib/stackwalker-2.0/lib -lcommon
	cp $@ $(LEAKS_LIBDIR)/
	chown $(USER):tools $(LEAKS_LIBDIR)/$@
	reln -g tools $(LEAKS_LIBDIR)/$@ $(LEAKS_LIBDIR)/$@.1


clean: 
	rm -rf *.o $(LEAKS_LIB)* 

veryclean: clean
	rm -rf $(LEAKS_LIBDIR)/$(LEAKS_LIB)*

