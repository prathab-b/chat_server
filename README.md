# Chat Server

## Description

A non-blocking, multi-client chat server implemented in C for Linux. The server uses TCP sockets and epoll to efficiently handle multiple simultaneous client connections. A Node.js WebSocket proxy is included to allow browser-based or WebSocket clients to communicate with the C-based TCP server in real time.

## Tech Stack

**Language (Server):** C <br>
**Platform:** Linux <br>
**Networking:** TCP Sockets <br>
**Proxy:** Node.js + WebSockets (ws)

## Features

- Non-blocking, event-driven server using epoll
- Supports multiple simultaneous clients
- User registration with unique usernames
- Direct (private) messaging between users
- Join and disconnect notifications
- Graceful client disconnect handling
- WebSocket-to-TCP proxy for browser-based clients

## How To Run

**C Chat Server**

To create the chat server, in a terminal, run the command:

`gcc chat_server.c -o chat_server`

Followed by:

`./chat_server <port>` <br>
Example: `./chat_server 8080` <br>
Note: If a port does not work, another port can be used.

**WebSocket Proxy**

The WebSocket proxy allows browser or WebSocket clients to connect to the C server. This requires:
- Node.js (v10+)
- npm

To install dependencies, run the command:

`npm install`

Then, in another terminal, run the proxy:

`node chat_server_proxy.js`

## Client Protocol

**Register / Join**

To register someone, open `chat_server_frontend.html` in a browser and enter a username when prompted.

The server will respond accordingly and alert everyone currently in the server with one of the following:

- `WELCOME <username>` – successful registration
- `USERNAME TAKEN` – username already in use, in which case the browser tab must be closed and `chat_server_frontend.html` must be reopened
- `INVALID JOIN` – invalid command format (reserved for malformed or non-compliant clients)

**Sending Messages**

To send a message:

- Enter the recipient's username
- Write the message in the message bar
- Press the enter key on the keyboard or the `Send` button

The server will respond accordingly and display one of the following:

`<sender>: <message>` – message delivered
`USER NOT FOUND` – recipient does not exist
`REGISTER FIRST` – client attempted to send a message before registering (should not occur during normal browser usage)
`INVALID MESSAGE` – malformed MESSAGE command (reserved for non-compliant or malformed clients)

**Quitting**

To quit, the user can simply close the browser tab.

## Future Improvements

Some potential future improvements for the server include:

- Channel-based messaging
- Authentication and password support
- Chat rooms
- Message history and logging
- Encrypted connections

## License

This project is licensed under the MIT License.
