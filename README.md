# Cli_chat

Command line group messagging client/server build with [OpenSSL](https://github.com/openssl/openssl.git) for socket cryptography

## How to use
```bash
# Compile:
make

# Run as server
./bin/clichat s <PORT>
# Run as client
./bin/clichat c <IPv4> <PORT>

# Local testing
make server
make client

```