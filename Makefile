CC = gcc
CFLAGS = -Wall -std=gnu99 -g -O1

PROGRAMS = memgrind memtest correct errors

all: $(PROGRAMS)

mymalloc.o: mymalloc.c mymalloc.h
	$(CC) $(CFLAGS) -c mymalloc.c

%.o: %.c mymalloc.h
	$(CC) $(CFLAGS) -c $<

memgrind: memgrind.o mymalloc.o
	$(CC) $(CFLAGS) -o $@ $^

memtest: memtest.o mymalloc.o
	$(CC) $(CFLAGS) -o $@ $^

correct: correct.o mymalloc.o
	$(CC) $(CFLAGS) -o $@ $^

errors: errors.o mymalloc.o
	$(CC) $(CFLAGS) -o $@ $^

# Run every test and check exit statuses.
test: all
	./memtest
	./correct
	@for n in 1 2 3; do \
	    ./errors $$n; s=$$?; \
	    if [ $$s -eq 2 ]; then echo "PASS: error case $$n exited with 2"; \
	    else echo "FAIL: error case $$n exited with $$s"; exit 1; fi; \
	done
	@./errors 4 2>&1 | grep -q "128 bytes leaked in 2 objects" \
	    && echo "PASS: leak detected" || (echo "FAIL: leak not reported"; exit 1)
	@./errors 5 && echo "PASS: free(NULL) is a no-op" || (echo "FAIL: free(NULL)"; exit 1)
	@$(CC) $(CFLAGS) -DREALMALLOC -o memtest_real memtest.c && ./memtest_real \
	    && echo "PASS: memtest works with the real malloc" ; rm -f memtest_real
	./memgrind

clean:
	rm -rf *.o $(PROGRAMS) memtest_real *.dSYM

.PHONY: all test clean
