/*
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
*/

#include "WinsockBlockingSocket.h"

#include <stdexcept>

/*
Based on:
http://www.win32developer.com/tutorial/winsock/winsock_tutorial_1.shtm
*/

namespace zusi
{

	WinsockBlockingSocket::WinsockBlockingSocket(const char* ip_address, int port)
	{
		// Initialise Winsock
		if (WSAStartup(MAKEWORD(2, 2), &m_wsadat) != 0)
		{
			WSACleanup();
			throw std::runtime_error("Winsock error - Winsock initialization failed");
		}

		// Create our socket
		m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (m_socket == INVALID_SOCKET)
		{
			WSACleanup();
			throw std::runtime_error("Winsock error - Socket creation Failed!");
		}

		// Setup our socket address structure
		SOCKADDR_IN SockAddr;
		SockAddr.sin_port = htons(port);
		SockAddr.sin_family = AF_INET;
		inet_pton(AF_INET, ip_address, &(SockAddr.sin_addr));

		// Attempt to connect to server
		if (connect(m_socket, (SOCKADDR*)(&SockAddr), sizeof(SockAddr)) != 0)
		{
			WSACleanup();
			throw std::runtime_error("Failed to establish connection with server");
		}
	}

	WinsockBlockingSocket::~WinsockBlockingSocket()
	{
		// Shutdown our socket
		shutdown(m_socket, SD_SEND);

		// Close our socket entirely
		closesocket(m_socket);

		// Cleanup Winsock
		WSACleanup();
	}

	int WinsockBlockingSocket::ReadBytes(void* dest, int bytes)
	{
		int inDataLength = recv(m_socket, static_cast<char*>(dest), bytes, MSG_WAITALL);
		return inDataLength;
	}

	int WinsockBlockingSocket::WriteBytes(const void* src, int bytes)
	{
		int outDataLength = send(m_socket, static_cast<const char*>(src), bytes, 0);
		return outDataLength;
	}

}

