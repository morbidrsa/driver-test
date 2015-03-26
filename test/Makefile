CFLAGS = -Wall -Wextra -O2

test-reflect: test-reflect.c
	$(CC) $(CFLAGS) test-reflect.c -o test-reflect
clean:
	rm -f *.o test-reflect

test: test-reflect
	./test-reflect "This is a test"

