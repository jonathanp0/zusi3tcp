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

#include <iostream>
#include <vector>

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

int main(int argc, char** argv)
{
	//Create a socket for message printing
	zusi::DebugSocket debug_socket;

	//Create connection to server
	try {
		zusi::WinsockBlockingSocket tcp_socket("127.0.0.1", 1436);
	
		//Subscribe to Fuehrerstand Data
		zusi::ClientConnection con(&tcp_socket);
		std::vector<zusi::FuehrerstandData> fd_ids{ zusi::Fs_Geschwindigkeit, zusi::Fs_Motordrehzahl, zusi::Fs_DruckBremszylinder };
		std::vector<zusi::ProgData> prog_ids{ zusi::Prog_SimStart };
	
		//Subscribe to receive status updates about the above variables
		//Not subscribing to input events
		con.connect("DumpFtd", fd_ids, prog_ids, false);

		std::cout << "Zusi Version:" << con.getZusiVersion() << std::endl;
		std::cout << "Connection Info: " << con.getConnectionnfo() << std::endl;

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