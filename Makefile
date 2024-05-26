all: lbs

test: lbs
	echo "abcdefghijkl" | valgrind --leak-check=full --show-leak-kinds=all ./lbs

lbs: lbs.o
	gcc -o $@ $^

%.o: %.c
	cpplint --filter=-build/include_subdir,-readability/casting $^
	gcc -c -g -Wall -o $@ $^

clean:
	rm -f lbs.o lbs
