HOSTNAME=arch

NACL=./nacl-20110221/build/$(HOSTNAME)

# select ABI from $(NACL)/bin/okabi
ABI=amd64

# select CC from $(NACL)/bin/okc-$(ABI)
CC=gcc -m64 -O3 -fomit-frame-pointer -funroll-loops

NACL_FLAGS=-I$(NACL)/include -I$(NACL)/include/$(ABI)

# the libs should come from $(NACL)/bin/oklibs-$(ABI)
NACL_LIBS=$(NACL)/lib/$(ABI)/libnacl.a $(NACL)/lib/$(ABI)/randombytes.o $(NACL)/lib/$(ABI)/cpucycles.o -lrt -lm

all: t1 t2 t3 t4 t5

t1: t1.c utilities.o
	$(CC) $(NACL_FLAGS) t1.c utilities.o $(NACL_LIBS) -o t1

t2: t2.c utilities.o
	$(CC) $(NACL_FLAGS) t2.c utilities.o $(NACL_LIBS) -o t2

t3: t3.c utilities.o conf.o
	$(CC) $(NACL_FLAGS) t3.c utilities.o conf.o $(NACL_LIBS) -o t3

t4: t4.c utilities.o
	$(CC) $(NACL_FLAGS) t4.c utilities.o $(NACL_LIBS) -o t4

t5: t5.c utilities.o
	$(CC) $(NACL_FLAGS) t5.c utilities.o $(NACL_LIBS) -o t5

utilities.o: utilities.c
	$(CC) $(NACL_FLAGS) -c utilities.c -o utilities.o

conf.o: conf.c
	$(CC) $(NACL_FLAGS) -c conf.c -o conf.o

clean:
	rm -f utilities.o
	rm -f conf.o
	rm -f t1
	rm -f t2
	rm -f t3
	rm -f t4
	rm -f t5
