# Zusi 3 TCP Protocol Library

An implementation of the Zusi 3 TCP protocol in C++.

This library is currently in an *alpha* state. It should be possible to send/receive all Zusi messages, but there is only limited error checking and the API will change in the future. The ID enumerations are only partly populated.

[Read the API Documentation](http://jonathanp0.github.io/zusi3tcp/doc/html/)

## Classes
* `zusi::Socket` - Abstract interface for a network communications class
* `zusi::Node` - Message node. Has and ID, child attributes and nodes.
* `zusi::Attribute` - Message attribute. Has an ID, and some data.
* `zusi::ClientConnection` -  Encapsulates a connection to a Zusi 3 server. Negotiates a connection with the server and sends and recieves message for the application.
* `zusi::ServerConnection` -  Emulates a Zusi 3 server. Negotiates a connection with the client sends data updates.

## Samples
* dump_ftd - Connects to server, subscribes to Führerstand variables and displays the contents of received messages
* pfeil_and_go - Connects to server, sounds the horn, and opens the throttle
* server_emulator - Accepts a client connection and sends simulated Speed and Power data to the client

For an example of constructing a message to transmit, see the `ClientConnection::connect()` method.

## Platforms

Currently there is no byte-swapping so it will only work on little-endian architectures.

The library is portable to different platforms by implementing the `Socket` interface.
Two implementations are included - `WinsockBlockingSocket`, which uses the Windows socket library in blocking mode, and `DebugSocket`, which prints data to the console insted of sending it.

## License
    The MIT License
    
    Copyright (c) 2016 Jonathan Pilborough

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.