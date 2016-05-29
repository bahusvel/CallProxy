test:
	gcc cproxy.c -o test

run: test
	./test
