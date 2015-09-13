# Protocol A4

This is an improvement over Protocol A2. It adds a commit/reveal step to stop Bob from controlling the shared secret.

## Assumptions

* Alice and Bob know each others public keys (`Alice.pub` and `Bob.pub`).

## Protocol

* 1. Alice generates an ephemeral keypair and sends it to Bob (with authenticated encryption)
* 2. Bob generates an ephemeral keypair and a secret and sends it and a commitment of the secret to Alice.eph
* 3. Alice generates a secret and responds to Bobs Ephemeral key with it
* 4. Bob sends a reveal to Alice
* 5. Both parties erase their ephemeral keys and compute the shared secret by xoring

## Notes

* This handshake takes 4 messages.
* The protocol is no longer symmetric
* The commit/reveal step stops Bob from being able to control the secret.
