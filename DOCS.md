# What is this

`up` is a program to send a file directly from one computer to another using encryption to try to stop anyone else from being able to read the file or change it in transit.

# How does it work

## Keys

To do the encryption you need to generate a public/private keypair and add it to your addressbook.

* You should always keep your private key secret. If you think someone got it you should delete it and make a new keypair.
* You need to give your friend your public key, you can tell anyone about the public key.
* You need to add your friends public key to your address book.

## Encryption

Each time you send a file a new temporary key is created and the file is encrypted using that, not the keypairs from your address book: those are only used to establish the temporary key in secret.

The idea for this is that if someone could watch your traffic they wouldn't be able to decrypt it even if they found your private key afterwards.

For more technical details you can read the full specification of the protocols in the protocol documents.

## Sending

To send a file from one computer to another the program has to make a TCP connection. One person will act as the `server` and listen for an incoming connection. The other will act as the `client` and connect to them.

There is no 3rd party/company server in the middle so the person that acts as the server will need to do port-forwarding.

# How do you use it

After setting up an addressbook the command is:

`./up (client|server) (push|pull) myname yourname <filename>`

* `push` means to send a file to the other person, `pull` means you expect to receive one
* myname and yourname are looked up in the addressbook
* <file> is only provided when `push`ing.
