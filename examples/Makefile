
# MPILEAKS_LIB, MPILEAKS_LIBDIR
CC = mpicc
CFLAGS = -g -O0 -Wall -Werror

# Codes that use mpileaks can be relinked using these flags: 
# LDFLAGS = -Wl,-rpath,$(MPILEAKS_LIBDIR) -L$(MPILEAKS_LIBDIR) -lmpileaks
# However, this is not necessary, simply preload library at run time: 
# LD_PRELOAD=$(MPILEAKS_LIBDIR)/libmpileaks.so

EXAMPLES = tests

all: $(EXAMPLES)

$(EXAMPLES): $(addsuffix .c,$(EXAMPLES))

run: $(EXAMPLES)
	../scripts/srun-mpileaks -ppdebug -n2 tests

clean: 
	rm -rf *~ *.o $(EXAMPLES) testfile.[0-9]*
