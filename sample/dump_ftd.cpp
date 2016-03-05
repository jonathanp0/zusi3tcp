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

#include <iostream>

#include "Zusi3TCP.h"
#include "DebugSocket.h"
#include "WinsockBlockingSocket.h"

void parseDataMessage(const zusi::Node& msg)
{
	if (msg.getId() == zusi::MsgType_Fahrpult)
	{
		for (const zusi::Node* node : msg.nodes)
		{
			if (node->getId() == zusi::Cmd_DATA_FTD)
			{
				for (const zusi::Attribute* att : node->attributes)
				{
					float value = *(reinterpret_cast<float*>(att->data));
					std::cout << "FS Data " << att->getId() << ": " << value << std::endl;
				}
			}
		}
	}
}

int main(int argv, char** argv)
{
	//Create a socket for message printing
	DebugSocket debug_socket;

	//Create connection to server
	try {
		WinsockBlockingSocket tcp_socket("127.0.0.1", 1436);
	
		//Subscribe to Fuehrerstand Data
		zusi::ClientConnection con(&tcp_socket);
		std::vector<zusi::FuehrerstandData> fd_ids{ zusi::Fs_Geschwindigkeit, zusi::Fs_Motordrehzahl, zusi::Fs_DruckBremszylinder };
	
		con.connect(fd_ids);

		while (true)
		{
			zusi::Node msg;
			if (con.receiveMessage(msg))
			{
				std::cout << "Received message..." << std::endl;
				msg.write(debug_socket); //Print data to the console

				parseDataMessage(msg);
				std::cout << std::endl;
			}
			else
			{
				std::cout << "Error receiving message" << std::endl;
				break;
			}
		}

	}
	catch (std::runtime_error& e)
	{
		std::cout << "Network error: " << e.what() << std::endl;
		return 1;
	}


	return 0;
}