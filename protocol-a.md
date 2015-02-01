# Protocol A

# Assumption:

* Alice and Bob have each others public keys known.

# Goals:

* Share a randomly generated key `k` for use with symmetric encryption.

* (S1) Safety against traffic being passively captured
* (S2) Safety against traffic being passively captures and private keys released later.
* (S3) Safety against a man in the middle attack.
* (S4) Safety against replay attacks.

# Protocol:

* 1. Alice and Bob generate new ephemeral public/private keypairs.
* 2a. Alice encrypts Alice_eph_pub to bobs public key
* 2b. Bob encrypts Bob_eph_pub to alices public key
* 3. Alice and Bob send each other the encrypted ephemeral public keys
* 4. Alice generates a random key `k`
* 5. Alice encrypts k to bobs ephemeral key and sends this to him
* 6. Alice erases her ephemeral keypair
* 7. Bob decrypts it
* 8. Bob erases his ephemeral keypair

# Analysis:

(S1) is validated because messages are encrypted
(S2) is validated because the ephemeral keypairs are erased, alice and bobs private keys only enable an attacker to see the public keys used
(S3) is validated because a man in the middle would need alice or bobs private key to insert a fake ephemeral key
(S4) is validated since a replay attack would fail since alice (or bob) would have generated a new ephemeral key unable to decrypt the replayed traffic.

# Notes:

It is intersting that replay attacks are protected against despite no challenge-reponse.
