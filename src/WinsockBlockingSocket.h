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

#pragma once
#include "Zusi3TCP.h"

#include <Ws2tcpip.h>

namespace zusi
{

	/**
	* @brief Socket implemented using the built-in Windows Winsock networking functions.
	*
	* If using this class, link in ws2_32.lib.
	*/
	class WinsockBlockingSocket :
		public zusi::Socket
	{
	public:
		/**
		* @brief Construct a new socket and initiate a TCP connection
		* @param ip_address Null-terminated string of IP Address to connect to
		* @param port Port to connection to, usually 1436.
		* @throws std::runtime_error system error when creating socket
		*/
		WinsockBlockingSocket(const char* ip_address, int port);
		virtual ~WinsockBlockingSocket();

		virtual int ReadBytes(void* dest, int bytes);
		virtual int WriteBytes(const void* src, int bytes);

	private:

		WinsockBlockingSocket(WinsockBlockingSocket& other) {}

		WSADATA m_wsadat;
		SOCKET m_socket;
	};

}

