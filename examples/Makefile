.PHONY: all clean

CFLAGS = -Wall -Wextra -g -O0
LDFLAGS =

all: example-caf libtechnicallyalac.a libtechnicallyalac.so

libtechnicallyalac.a: technicallyalac.o
	$(AR) rcs $@ $^

libtechnicallyalac.so: technicallyalac.o
	$(CC) -shared -o $@ $^ $(LDFLAGS)

technicallyalac.o: technicallyalac.c
	$(CC) $(CFLAGS) -o $@ -c $^

example-caf: example-caf.o example-shared.o
	$(CC) -o $@ $^ $(LDFLAGS)

example-caf.o: example-caf.c ../technicallyalac.h
	$(CC) $(CFLAGS) -o $@ -c $<

example-shared.o: example-shared.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f example-caf example-caf.o example-shared.o libtechnicallyalac.a libtechnicallyalac.so technicallyalac.o
