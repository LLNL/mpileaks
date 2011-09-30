CC    = mpig++
OPT   = -g -O0

TOOLS = /usr/global/tools
MPILEAKS = $(TOOLS)/mpileaks

LEAKS_FLAGS = $(OPT) -fPIC
LEAKS_INCLUDES = -I$(MPILEAKS)/include -I/usr/global/tools/adept-utils/chaos_4_x86_64_ib/include
LEAKS_OBJS = mpileaks.o fileio.o
LEAKS_LIB = libmpileaks.so
LEAKS_LIB_DIR = $(HOME)/mpileaks/lib

EXAMPLES = example ex_pcomm
EXAMPLES2= example_preload 

all: $(LEAKS_LIB) $(EXAMPLES)

mpileaks.o: mpileaks.cpp 
	$(CC) $(LEAKS_FLAGS) $(LEAKS_INCLUDES) -c -o $@ mpileaks.cpp

fileio.o: fileio.cpp
	$(CC) $(LEAKS_FLAGS) $(LEAKS_INCLUDES) -c -o $@ fileio.cpp

$(LEAKS_LIB): $(LEAKS_OBJS)
	$(CC) $(LEAKS_FLAGS) -shared -Wl,-soname,$(LEAKS_LIB).1 -o $@ $(LEAKS_OBJS) \
	-Wl,-rpath,$(MPILEAKS)/lib -L$(MPILEAKS)/lib -lcallpath \
	-Wl,-rpath,$(TOOLS)/adept-utils/chaos_4_x86_64_ib/lib \
	-L$(TOOLS)/adept-utils/chaos_4_x86_64_ib/lib \
	-Wl,-rpath,$(TOOLS)/dyninst/stackwalker/chaos_4_x86_64_ib/stackwalker-2.0/lib \
	-L$(TOOLS)/dyninst/stackwalker/chaos_4_x86_64_ib/stackwalker-2.0/lib -lcommon
	reln -g leon $@ $(LEAKS_LIB).1
	cp $(LEAKS_LIB)* $(LEAKS_LIB_DIR)/

$(EXAMPLES): $(LEAKS_LIB) $(addsuffix .c,$(EXAMPLES))
	$(CC) -Wall -Werror $(OPT) -o $@ $@.c -Wl,-rpath,$(LEAKS_LIB_DIR) \
	-L$(LEAKS_LIB_DIR) -lmpileaks

example_preload: $(LEAKS_LIB)
	$(CC) $(OPT) -o example_preload example.c

clean:
	rm -f *.o $(EXAMPLES) $(EXAMPLES2) $(LEAKS_LIB)* $(LEAKS_LIB_DIR)/$(LEAKS_LIB)*
