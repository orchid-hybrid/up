# Protocol C

# Goals

* Send a file from one party to another
* Only have to transmit what we have to in order for the recipient to have the complete data
* Verify fidelity of recieved data

# Protocol

0. Alice and Bob decide on a block size S
1. Alice sends Bob a 4-byte unsigned integer representing the length of the file she is about to send to Bob
2. Alice sends Bob the name of the file she is sending, zero padded to 64 bytes
3. Alice reads at most S bytes from her input file, hashes them, and sends Bob the hash
4. Bob replies whether or not he has that block stored in his version of the file
5. If Bob replies that he does not have that block, then Alice sends him the block, if not, then Alice does nothing
6. Repeat steps 3-5 for each block in the input file

# Notes

* This protocol sits on top of the key exchange and protocol B, so it is intended to be used over an encrypted channel
* Bob will know, based on the length of the file as transmitted in step 1, if a block he is about to recieve will be less than S bytes in length - so we don't have to have a special case for that, or use padding