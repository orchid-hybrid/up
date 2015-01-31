HOSTNAME=`hostname`

NACL=./nacl-20110221/build/$(HOSTNAME)

# select ABI from $(NACL)/bin/okabi
ABI=amd64

# select CC from $(NACL)/bin/okc-$(ABI)
CC=gcc -m64 -O3 -fomit-frame-pointer -funroll-loops

NACL_FLAGS=-I$(NACL)/include -I$(NACL)/include/$(ABI)

# the libs should come from $(NACL)/bin/oklibs-$(ABI)
NACL_LIBS=$(NACL)/lib/$(ABI)/libnacl.a $(NACL)/lib/$(ABI)/randombytes.o $(NACL)/lib/$(ABI)/cpucycles.o -lrt -lnsl -lm

all: t1 t2

t1: t1.c utilities.o
	$(CC) $(NACL_FLAGS) t1.c utilities.o $(NACL_LIBS) -o t1

t2: t2.c utilities.o conf.o
	$(CC) $(NACL_FLAGS) t2.c utilities.o $(NACL_LIBS) -o t2

utilities.o: utilities.c
	$(CC) $(NACL_FLAGS) -c utilities.c -o utilities.o

conf.o: conf.c
	$(CC) $(NACL_FLAGS) -c conf.c -o conf.o

clean:
	rm -f utilities.o
	rm -f conf.o
	rm -f t1.o
