CC    = mpiicc
OPT   = -g -O0

all: clean
	$(CC) $(OPT) -fPIC -c -o mpileaks.o mpileaks.cpp -I../include -I/usr/global/tools/adept-utils/chaos_4_x86_64_ib/include
	$(CC) $(OPT) -fPIC -shared -Wl,-soname,libmpileaks.so.1 -o libmpileaks.so mpileaks.o -Wl,-rpath,/usr/global/tools/mpileaks/lib -L/usr/global/tools/mpileaks/lib -lcallpath -Wl,-rpath,/usr/global/tools/adept-utils/chaos_4_x86_64_ib/lib -L/usr/global/tools/adept-utils/chaos_4_x86_64_ib/lib -Wl,-rpath,/usr/global/tools/dyninst/stackwalker/chaos_4_x86_64_ib/stackwalker-2.0/lib -L/usr/global/tools/dyninst/stackwalker/chaos_4_x86_64_ib/stackwalker-2.0/lib -lcommon
	reln libmpileaks.so libmpileaks.so.1
#	cp libmpileaks.so* ../lib/.
#	$(CC) $(OPT) -o example example.c -Wl,-rpath,/usr/global/tools/mpileaks/lib -L/usr/global/tools/mpileaks/lib -lmpileaks
#	$(CC) $(OPT) -o example_preload example.c
#	sp -g tools /usr/global/tools/mpileaks

clean:
	rm -f *.o example
