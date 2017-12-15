CFLAGS= -Wall -Wextra -O3

all: data.tar

clean:
	rm -f data.tar test_driver *.o

data.tar: test_driver
	tar cf data.tar test_driver

test_driver: test_driver.o
