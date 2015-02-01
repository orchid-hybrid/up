# Protocol A

# Assumption:

* Alice and Bob know each other's public key.

# Goals:

* Share a randomly generated key `k` for use with symmetric encryption.

* (S1) Safety against traffic being passively captured
* (S2) Safety against traffic being passively captures and private keys released later.
* (S3) Safety against a man in the middle attack.
* (S4) Safety against replay attacks.
* (S5) Safety against a single party having a compromised random number generator.

# Protocol:

* 1. Alice and Bob generate new ephemeral keypairs
* 2a. Alice encrypts her ephemeral public key with her non-ephemeral private key and Bob's non-ephemeral public key
* 2b. Bob encrypts his ephemeral public key with his non-ephemeral private key and Alice's non-ephemeral public key
* 3. Alice and Bob send each other their encrypted ephemeral public keys
* 4. Alice and Bob both generate half of a symmetric key
* 5a. Alice encrypts her half of the symmetric key with her ephemeral private key and Bob's ephemeral public key, and sends this to him
* 5b. Bob encrypts his half of the symmetric key with his ephemeral private key and Alice's ephemeral public key, and sends this to her
* 6. Alice and Bob decrypt their respective halves of the symmetric key
* 7. Alice and Bob erase their ephemeral keypairs

# Analysis:

* (S1) is validated because messages are encrypted.
* (S2) is validated because the ephemeral keypairs are erased, Alice and Bob's private keys only enable an attacker to see the public keys used.
* (S3) is validated because a man in the middle would need Alice or Bob's private key to insert a fake ephemeral key.
* (S4) is validated since a replay attack would fail since Alice (or Bob) would have generated a new ephemeral key unable to decrypt the replayed traffic.
* (S5) is validated, the fact that each party only generates half of the symmetric key should protect against one party having a compromised RNG.

# Notes:

It is interesting that replay attacks are protected against despite no challenge-response.