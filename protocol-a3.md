# Protocol A3

This is an improvement over Protocol A2. It adds a commit/reveal step to stop Bob from controlling the shared secret.

## Assumptions

* Alice and Bob know each others public keys (`Alice.pub` and `Bob.pub`).

## Protocol

* 1. Alice and Bob generate ephemeral keypairs (`Alice.eph` and `Bob.eph`).
* 2. Alice and Bob exchange the ephemeral public keys, encrypted and authenticated between their public keys.
* 3. Alice and Bob generate a random secret and create a commitment to it
* 4. Alice and Bob exchange their commitments, encrypted and authenticated between their ephemeral public keys.
* 5. Alice and Bob exchange their reveals, encrypted and authenticated between their ephemeral public keys.
* 6. Alice and Bob erase their ephemeral keys.
* 7. Alice and Bob verify the commitments were valid.
* 8. Alice and Bob compute the shared secret by xoring each of their random secret.

## Notes

* This handshake takes 6 messages.
* The commit/reveal step stops Bob from being able to control the secret.
