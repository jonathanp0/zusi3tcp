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

#include "DebugSocket.h"

#include <iostream>
#include <iomanip>

namespace zusi
{

	DebugSocket::DebugSocket()
	{
	}

	DebugSocket::~DebugSocket()
	{
	}

	int DebugSocket::ReadBytes(void* dest, int bytes)
	{
		return -1;
	}

	int DebugSocket::WriteBytes(const void* src, int bytes)
	{
		const unsigned char* src_chars = static_cast<const unsigned char*>(src);
		const std::ios_base::fmtflags flags = std::cout.flags();

		std::cout << std::setw(2) << bytes << "b ";
		std::cout << std::hex << std::setfill('0');

		for (int i = 0; i < bytes; ++i)
			std::cout << std::setw(2) << static_cast<unsigned int>(*(src_chars + i));

		std::cout << std::endl;
		std::cout.setf(flags);

		return bytes;
	}

}
