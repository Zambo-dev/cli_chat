# Cli_chat

Command line group messagging client/server build with [OpenSSL](https://github.com/openssl/openssl.git) for socket cryptography

## How to use
Flags:
- -t < s/c > define the socket type 's' for server or 'c' for client
- -l < file.conf > load config file
- -c \<username> \<ip> \<port> \<cert.pem> \<key.pem> 
  - to leave a parameter blank leave a space between double quotes
  - add '-s < filename.conf >' to save the custom configuration 

