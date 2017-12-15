CFLAGS= -Wall -Wextra -O3

all: data.tar pipelines.so

clean:
	rm -f data.tar test_driver pipelines.so *.o

data.tar: test_driver
	tar cf data.tar test_driver

test_driver: test_driver.o

pipelines.so: pls.o
	$(CC) -shared -o $@ $^

pls.o: CFLAGS+= -fpic
