LIBS        = -lm
INC_PATH    = -I.
CC_FLAGS    = -g

all: pa31 pa32

pa31: testsuite.c pa31.c
	cc $(CC_FLAGS) $(INC_PATH) -o pa31 testsuite.c $(LIBS)

pa32: testsuite.c pa32.c
	cc -DVHEAP $(CC_FLAGS) $(INC_PATH) -o pa32 testsuite.c $(LIBS)

clean :
	rm -f *.o *~ $(PROGS)
