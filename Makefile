HOSTNAME=`hostname -s`

NACL=./nacl-20110221/build/$(HOSTNAME)

# select ABI from $(NACL)/bin/okabi
ABI=amd64

# select CC from $(NACL)/bin/okc-$(ABI)
#CC=gcc -m64 -O3 -fomit-frame-pointer -funroll-loops
CC=gcc -g -m64 -fsanitize=address -fno-omit-frame-pointer

NACL_FLAGS=-I$(NACL)/include -I$(NACL)/include/$(ABI)

# the libs should come from $(NACL)/bin/oklibs-$(ABI)
NACL_LIBS=$(NACL)/lib/$(ABI)/libnacl.a $(NACL)/lib/$(ABI)/randombytes.o $(NACL)/lib/$(ABI)/cpucycles.o -lrt -lm

all: t1 t2 t3 t4 t5 t6 t7 t8 up

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

t6: t6.c utilities.o conf.o padded_array.o
	$(CC) $(NACL_FLAGS) t6.c utilities.o conf.o padded_array.o $(NACL_LIBS) -o t6

t7: t7.c network.o
	$(CC) $(NACL_FLAGS) t7.c network.o $(NACL_LIBS) -o t7

t8: t8.c utilities.o network.o padded_array.o protocol.o
	$(CC) $(NACL_FLAGS) t8.c utilities.o network.o padded_array.o protocol.o $(NACL_LIBS) -o t8

up: up.c utilities.o conf.o network.o padded_array.o protocol.o
	$(CC) $(NACL_FLAGS) up.c utilities.o conf.o network.o padded_array.o protocol.o $(NACL_LIBS) -o up

keys: t1
	mkdir -p keys
	./t1
	mv secret_key.seckey keys/alice.seckey
	mv public_key.pubkey keys/alice.pubkey
	./t1
	mv secret_key.seckey keys/bob.seckey
	mv public_key.pubkey keys/bob.pubkey
	./t1
	mv secret_key.seckey keys/eve.seckey
	mv public_key.pubkey keys/eve.pubkey

addressbook.conf: keys
	rm -f addressbook.conf
	echo "alice keys/alice.pubkey keys/alice.seckey" >> addressbook.conf
	echo "bob keys/bob.pubkey keys/bob.seckey" >> addressbook.conf
	echo "eve keys/eve.pubkey" >> addressbook.conf

utilities.o: utilities.c
	$(CC) $(NACL_FLAGS) -c utilities.c -o utilities.o

conf.o: conf.c conf.h
	$(CC) $(NACL_FLAGS) -c conf.c -o conf.o

network.o: network.c network.h
	$(CC) $(NACL_FLAGS) -c network.c -o network.o

protocol.o: padded_array.o
	$(CC) $(NACL_FLAGS) -c protocol.c -o protocol.o

padded_array.o: padded_array.c padded_array.h
	$(CC) $(NACL_FLAGS) -c padded_array.c -o padded_array.o

clean:
	rm -f utilities.o
	rm -f conf.o
	rm -f network.o
	rm -f padded_array.o
	rm -f t1
	rm -f t2
	rm -f t3
	rm -f t4
	rm -f t5
	rm -f t6
	rm -f t7
	rm -f up
