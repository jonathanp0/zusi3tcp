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
#include <chrono>
#include <thread>

#include "Zusi3TCP.h"
#include "DebugSocket.h"
#include "WinsockBlockingSocket.h"


int main(int argc, char** argv)
{
	//Create a socket for message printing
	zusi::DebugSocket debug_socket;

	//Create connection to server
	try {
		zusi::WinsockBlockingSocket tcp_socket("127.0.0.1", 1436);

		zusi::ClientConnection con(&tcp_socket);
		std::vector<zusi::FuehrerstandData> fd_ids;
		std::vector<zusi::ProgData> prog_ids;
		con.connect("PfeilAndGo", fd_ids, prog_ids, false);

		//Pfeil
		con.sendInput(zusi::Tt_Pfeife, zusi::Tk_PfeifeDown, zusi::Ta_Down, 1);
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		con.sendInput(zusi::Tt_Pfeife, zusi::Tk_PfeifeUp, zusi::Ta_Up, 0);

		//Fahrschalter 1->5
		for (int i = 1; i <= 5; ++i)
		{
			con.sendInput(zusi::Tt_Fahrschalter, zusi::Tk_Unbestimmt, zusi::Ta_Absolut, i);
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}

		//Fahrschalter 5->10
		for (int i = 0; i < 5; ++i)
		{
			con.sendInput(zusi::Tt_Fahrschalter, zusi::Tk_FahrschalterAuf_Down, zusi::Ta_AufDown, 1);
			std::this_thread::sleep_for(std::chrono::milliseconds(2000));
			con.sendInput(zusi::Tt_Fahrschalter, zusi::Tk_FahrschalterAuf_Up, zusi::Ta_AufUp, 0);
		}


	}
	catch (std::runtime_error& e)
	{
		std::cout << "Network error: " << e.what() << std::endl;
		return 1;
	}


	return 0;
}