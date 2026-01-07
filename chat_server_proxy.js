const WebSocket = require('ws'); // ws: Node.js WebSocket library for browser communication.
const net = require('net'); // net: Node's built-in module for TCP socket communication.

const wss = new WebSocket.Server({ port: 45291 }); // Creates a WebSocket server listening on port 1010.

wss.on('connection', function connection(ws) // This runs each time a browser connects to the WebSocket
{
    const tcpSocket = new net.Socket(); // This creates a new TCP socket to connect to the C server.

    tcpSocket.connect(1234, '127.0.0.1', () => { // This connects the TCP socket to the C server
        console.log('Connected to C server');    // It assumes the C server is listening on port 1000
                                                 // and on localhost (127.0.0.1)
    });

    // From C server -> Browser
    tcpSocket.on('data', (data) => {
        ws.send(data.toString()); // When C server sends a message, it gets forwarded via ws.send() to the browser.
    });

    // From Browser -> C server
    ws.on('message', (message) => {
        tcpSocket.write(message + '\n'); // When the browser sends a message,
                                         // it's forwarded as a raw TCP line to the C server.
    });

    ws.on('close', () => { // If the browser disconnects, close the TCP connection to the C server.
        tcpSocket.end();
    });

    tcpSocket.on('close', () => { // If the C server dies, disconnect the browser.
        ws.close();
    });

    tcpSocket.on('error', (err) => { // Errors are logged.
        console.error('TCP Error:', err);
    });
});