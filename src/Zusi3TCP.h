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

#pragma once

#include <cstdint>
#include <vector>

namespace zusi
{
	enum MsgType
	{
		MsgType_Connecting = 1,
		MsgType_Fahrpult = 2
	};

	enum Command
	{
		Cmd_HELLO = 1,
		Cmd_ACK_HELLO = 2,
		Cmd_NEEDED_DATA = 3,
		Cmd_ACK_NEEDED_DATA = 4,
		Cmd_DATA_FTD = 0xA,
		Cmd_DATA_OPERATION = 0xB,
		Cmd_DATA_PROG = 0xC,
		Cmd_INPUT = 0x010A,
		Cmd_CONTROL = 0x010B,
		Cmd_GRAPHIC = 0x010C
	};

	enum FuehrerstandData
	{
		Fs_Geschwindigkeit = 1,
		Fs_DruckHauptlufleitung = 2,
		Fs_DruckBremszylinder = 3,
		Fs_DruckHauptluftbehaelter = 4,
		Fs_Motordrehzahl = 15
	};

	/**
	* Abstract interface for a socket
	*/
	class Socket
	{
	public:

		/** Try to read bytes into dest from socket. Return number of bytes actually written
		* Method should block until all requested bytes are read, or the stream ends
		*/
		virtual int ReadBytes(void* dest, int bytes) = 0;
		/**
		* Try to write bytes from src to socket. Return number of bytes actually written 
		*/
		virtual int WriteBytes(const void* src, int bytes) = 0;
	};

	/**
	* Generic Zusi message attribute.
	* data is owned by the class
	*/
	class Attribute
	{
	public:

		Attribute() : m_id(0), data_bytes(0)
		{
		}

		Attribute(uint16_t id) : m_id(id), data_bytes(0)
		{
		}

		virtual ~Attribute()
		{
			if (data_bytes > 0)
				delete data;
		}

		void write(Socket& sock) const;

		bool read(Socket& sock, uint32_t length);

		uint16_t getId() const
		{
			return m_id;
		}

		/**
		* Utility function to set the value as SmallInt
		*/
		void setValueUint16(uint16_t value)
		{
			data = new uint16_t(value);
			data_bytes = sizeof(value);
		}

		uint32_t data_bytes;
		void* data;

	private:
		uint16_t m_id;
	};

	/**
	* Generic Zusi message node
	*/
	class Node
	{
	public:

		Node() : m_id(0)
		{
		}

		Node(uint16_t id) : m_id(id)
		{
		}

		virtual ~Node()
		{
			for (Attribute* a_p : attributes)
				delete a_p;

			for (Node* n_p :  nodes)
				delete n_p;
		}

		bool write(Socket& sock) const;
		
		bool read(Socket& sock);

		uint16_t getId() const
		{
			return m_id;
		}

		std::vector<Attribute*> attributes;
		std::vector<Node*> nodes;

	private:
		uint16_t m_id;

		static const uint32_t NODE_START = 0;
		static const uint32_t NODE_END = 0xFFFFFFFF;
	};

	/**
	* Parent class for a connection
	*/
	class Connection
	{
	public:
		/**
		* Create connection which will communicate over socket
		* @param socket The socket - class does not take ownership of it
		*/
		Connection(Socket* socket) : m_socket(socket)
		{
		}

		virtual ~Connection()
		{
		}

		/**
		* Receive a message
		*/
		bool receiveMessage(Node& dest) const;

		/**
		* Send a message
		*/
		bool sendMessage(Node& src);

	protected:
		Socket* m_socket;
	};

	/**
	* Manages connection to a Zusi server
	*/
	class ClientConnection : public Connection
	{
	public:
		ClientConnection(Socket* socket) : Connection(socket)
		{
		}

		virtual ~ClientConnection()
		{
		}

		/**
		* Set up connection to server
		* @param fs_data Fuehrerstand Data ID's to subscribe to
		*/
		bool connect(const std::vector<FuehrerstandData> fs_data);

	};

}

