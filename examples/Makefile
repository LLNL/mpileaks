
# MPILEAKS_LIB, MPILEAKS_LIBDIR
include ../defs.mk

CC = mpicc
CFLAGS = -g -Wall -Werror

# Codes that use mpileaks can be relinked using these flags: 
# LDFLAGS = -Wl,-rpath,$(MPILEAKS_LIBDIR) -L$(MPILEAKS_LIBDIR) -lmpileaks
# However, this is not necessary, simply preload library at run time: 
# LD_PRELOAD=$(MPILEAKS_LIBDIR)/libmpileaks.so

EXAMPLES = tests

all: $(EXAMPLES)

$(EXAMPLES): $(addsuffix .c,$(EXAMPLES))

run: $(EXAMPLES)
	MPILEAKS_SO=$(MPILEAKS_LIBDIR)/$(MPILEAKS_LIB) ../scripts/srun-mpileaks -ppdebug -n2 tests

clean: 
	rm -rf *~ *.o $(EXAMPLES) testfile.[0-9]*
