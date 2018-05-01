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

auto const NS_INADDRSZ = 4;
auto const NS_IN6ADDRSZ = 16;
auto const NS_INT16SZ = 2;

int inet_pton4(const char *src, char *dst)
{
    uint8_t tmp[NS_INADDRSZ], *tp;

    int saw_digit = 0;
    int octets = 0;
    *(tp = tmp) = 0;

    int ch;
    while ((ch = *src++) != '\0')
    {
        if (ch >= '0' && ch <= '9')
        {
            uint32_t n = *tp * 10 + (ch - '0');

            if (saw_digit && *tp == 0)
                return 0;

            if (n > 255)
                return 0;

            *tp = n;
            if (!saw_digit)
            {
                if (++octets > 4)
                    return 0;
                saw_digit = 1;
            }
        }
        else if (ch == '.' && saw_digit)
        {
            if (octets == 4)
                return 0;
            *++tp = 0;
            saw_digit = 0;
        }
        else
            return 0;
    }
    if (octets < 4)
        return 0;

    memcpy(dst, tmp, NS_INADDRSZ);

    return 1;
}

int inet_pton6(const char *src, char *dst)
{
    static const char xdigits[] = "0123456789abcdef";
    uint8_t tmp[NS_IN6ADDRSZ];

    uint8_t *tp = (uint8_t*) memset(tmp, '\0', NS_IN6ADDRSZ);
    uint8_t *endp = tp + NS_IN6ADDRSZ;
    uint8_t *colonp = NULL;

    /* Leading :: requires some special handling. */
    if (*src == ':')
    {
        if (*++src != ':')
            return 0;
    }

    const char *curtok = src;
    int saw_xdigit = 0;
    uint32_t val = 0;
    int ch;
    while ((ch = tolower(*src++)) != '\0')
    {
        const char *pch = strchr(xdigits, ch);
        if (pch != NULL)
        {
            val <<= 4;
            val |= (pch - xdigits);
            if (val > 0xffff)
                return 0;
            saw_xdigit = 1;
            continue;
        }
        if (ch == ':')
        {
            curtok = src;
            if (!saw_xdigit)
            {
                if (colonp)
                    return 0;
                colonp = tp;
                continue;
            }
            else if (*src == '\0')
            {
                return 0;
            }
            if (tp + NS_INT16SZ > endp)
                return 0;
            *tp++ = (uint8_t) (val >> 8) & 0xff;
            *tp++ = (uint8_t) val & 0xff;
            saw_xdigit = 0;
            val = 0;
            continue;
        }
        if (ch == '.' && ((tp + NS_INADDRSZ) <= endp) &&
                inet_pton4(curtok, (char*) tp) > 0)
        {
            tp += NS_INADDRSZ;
            saw_xdigit = 0;
            break; /* '\0' was seen by inet_pton4(). */
        }
        return 0;
    }
    if (saw_xdigit)
    {
        if (tp + NS_INT16SZ > endp)
            return 0;
        *tp++ = (uint8_t) (val >> 8) & 0xff;
        *tp++ = (uint8_t) val & 0xff;
    }
    if (colonp != NULL)
    {
        /*
         * Since some memmove()'s erroneously fail to handle
         * overlapping regions, we'll do the shift by hand.
         */
        const int n = tp - colonp;

        if (tp == endp)
            return 0;

        for (int i = 1; i <= n; i++)
        {
            endp[-i] = colonp[n - i];
            colonp[n - i] = 0;
        }
        tp = endp;
    }
    if (tp != endp)
        return 0;

    memcpy(dst, tmp, NS_IN6ADDRSZ);

    return 1;
}

#define NS_INADDRSZ  4
#define NS_IN6ADDRSZ 16
#define NS_INT16SZ   2

int inet_pton(int af, const char *src, char *dst)
{
    switch (af)
    {
    case AF_INET:
        return inet_pton4(src, dst);
    case AF_INET6:
        return inet_pton6(src, dst);
    default:
        return -1;
    }
}
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
		inet_pton(AF_INET, ip_address, (char*)&(SockAddr.sin_addr));

		// Attempt to connect to server
		if (connect(m_socket, (SOCKADDR*)(&SockAddr), sizeof(SockAddr)) != 0)
		{
			WSACleanup();
			throw std::runtime_error("Failed to establish connection with server");
		}
	}

	WinsockBlockingSocket::WinsockBlockingSocket(SOCKET socket) : m_socket(socket)
	{
		// Initialise Winsock
		if (WSAStartup(MAKEWORD(2, 2), &m_wsadat) != 0)
		{
			WSACleanup();
			throw std::runtime_error("Winsock error - Winsock initialization failed");
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

	bool WinsockBlockingSocket::DataToRead()
	{
		unsigned long bytes_available;
		if(ioctlsocket(m_socket, FIONREAD, &bytes_available) != 0)
			return false;
		return bytes_available > 0;
	}
}

