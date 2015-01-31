NACL=./nacl-20110221/build/arch

# select ABI from $(NACL)/bin/okabi
ABI=amd64

# select CC from $(NACL)/bin/okc-$(ABI)
CC=gcc -m64 -O3 -fomit-frame-pointer -funroll-loops

NACL_FLAGS=-I$(NACL)/include -I$(NACL)/include/$(ABI)

# the libs should come from $(NACL)/bin/oklibs-$(ABI)
NACL_LIBS=$(NACL)/lib/$(ABI)/libnacl.a $(NACL)/lib/$(ABI)/randombytes.o $(NACL)/lib/$(ABI)/cpucycles.o -lrt -lnsl -lm

all: t1

t1: t1.c utilities.o
	$(CC) $(NACL_FLAGS) t1.c utilities.o $(NACL_LIBS) -o t1

utilities.o: utilities.c
	$(CC) $(NACL_FLAGS) -c utilities.c -o utilities.o

clean:
	rm -f utilities.o
	rm -f t1.o
