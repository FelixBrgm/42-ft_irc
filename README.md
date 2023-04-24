# IRC Server in C++ 98

## Overview

This README provides instructions for setting up and running an IRC server implemented in C++ 98. The server is designed to handle multiple clients without hanging and uses non-blocking I/O operations.

## Requirements

* C++ 98 compiler
* An IRC client (irssi recommended)

## Usage

To compile and run the server, execute the following commands:

```
make
./ircserv <port> <password>
```

* `port`: The port number on which your IRC server will be listening for incoming IRC connections.
* `password`: The connection password required by any IRC client trying to connect to your server.

## Features

* Authenticate, set a nickname, and a username
* Join a channel
* Send and receive private messages
* Forward messages from one client to all other clients in the channel
* Support for operators and regular users

### Channel Operator Commands

* `KICK`: Eject a client from the channel
* `INVITE`: Invite a client to a channel
* `TOPIC`: Change or view the channel topic
* `MODE`: Change the channel's mode
  * `i`: Set/remove Invite-only channel
  * `t`: Set/remove the restrictions of the TOPIC command to channel operators
  * `k`: Set/remove the channel key (password)
  * `o`: Give/take channel operator privilege
  * `l`: Set/remove the user limit to the channel

## Evaluation

Your reference client should be able to connect to the server without encountering any errors. The server's functionality with the reference client should be similar to using it with any official IRC server. Only the features listed above are required to be implemented.

## Code Quality

It is expected that you write clean and well-structured code. Using `read/recv` or `write/send` functions without `poll()` (or equivalent) is not allowed and will result in a grade of 0. Only one `poll()` (or equivalent) can be used for handling all I/O operations, including read, write, listen, and so forth.

## Notes

* The server must be capable of handling multiple clients simultaneously and never hang.
* Forking is not allowed.
* All I/O operations must be non-blocking.
* Communication between the client and server must be done via TCP/IP (v4 or v6).
