all: clean program
	@rm -f *.o

clean:
	@rm -f *.o
	@rm -f *.a

program:
	$(CC) $(CFLAGS) -o $@ $^
