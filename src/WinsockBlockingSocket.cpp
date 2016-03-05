/*
Copyright(c) 2016, Jonathan Pilborough
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met :
*Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and / or other materials provided with the distribution.
* Neither the name of the author nor the
names of its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "WinsockBlockingSocket.h"

#include <stdexcept>

/*
Based on:
http://www.win32developer.com/tutorial/winsock/winsock_tutorial_1.shtm
*/

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

