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

#include "DebugSocket.h"

#include <iostream>
#include <iomanip>

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
