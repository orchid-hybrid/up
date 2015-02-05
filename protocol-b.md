# Protocol B

# Goals:

Initiate the connection and work out who is sending a file to who.

# Protocol:

Assume Alice is acting as a server and Bob as a client.

* 1. Alice listens for an incoming connection
* 2. Bob connects to Alice
* 3. Alice sends a message "PUSH" or "PULL" to Bob depending on whether she expects to upload a file to him or recieve one.
* 4. Bob recieves four bytes
* 5. Bob sends a message "PUSH" or "PULL" to Alice depending on whether he expects to upload a file to him or recieve one.
* 6. Alice recieves four bytes
* 7. Alice and Bob must check that the message recieved is PUSH or PULL and that it is the opposite of what they sent, otherwise disconnect.

# Notes:

This is performed entirely in plaintext. There is no reason to encrypt it since anyone watching the traffic would be able to see who is sending more data to the other.
