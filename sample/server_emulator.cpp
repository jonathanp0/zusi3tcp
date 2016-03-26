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
#include <string>
#include <chrono>
#include <thread>
#include <vector>

#include "Zusi3TCP.h"
#include "WinsockBlockingSocket.h"


int main(int argc, char** argv)
{

	try {
		
		WSADATA wsadat;
		if (WSAStartup(MAKEWORD(2, 2), &wsadat) != 0)
			throw std::runtime_error("Winsock startup failed");

		SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (listen_socket == INVALID_SOCKET)
			throw std::runtime_error("Socket creation failed.");

		SOCKADDR_IN bindTo;
		bindTo.sin_family = AF_INET;
		bindTo.sin_addr.s_addr = INADDR_ANY;
		bindTo.sin_port = htons(1436);

		if (bind(listen_socket, (SOCKADDR*)(&bindTo), sizeof(bindTo)) == SOCKET_ERROR)
			throw std::runtime_error("Unable to bind socket!");

		listen(listen_socket, 1);

		SOCKET client_raw_socket = SOCKET_ERROR;
		while (client_raw_socket == SOCKET_ERROR)
		{
			std::cout << "Waiting for incoming connections...\r\n";
			client_raw_socket = accept(listen_socket, NULL, NULL);
		}

		std::cout << "Client connected\r\n";

		zusi::WinsockBlockingSocket client_socket(client_raw_socket);

		zusi::ServerConnection con(&client_socket);

		//Negotiate the connection - HELLO & NEEDED_DATA commands
		con.accept();

		std::cout << "Client: " << con.getClientName() << " Version: " << con.getClientVersion() << std::endl;

		std::vector<std::pair<zusi::FuehrerstandData, float>> fake_data;

		//Speed 0 km/h -> 160 km/h Power 0 A ->450 A
		float speed = 0.0f;
		for (float speed = 0.0f; speed < 45.0f; speed += 0.5f)
		{
			fake_data.push_back(std::make_pair(zusi::Fs_Geschwindigkeit, speed));
			fake_data.push_back(std::make_pair(zusi::Fs_Oberstrom, speed*10.0f));
			con.sendData(fake_data);
			fake_data.clear();
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}

		// Shutdown our socket
		shutdown(listen_socket, SD_SEND);
		closesocket(listen_socket);

		// Cleanup Winsock
		WSACleanup();
		
	}
	catch (std::runtime_error& e)
	{
		std::cout << "Network error: " << e.what() << std::endl;
		WSACleanup();
		return 1;
	}

	return 0;
}