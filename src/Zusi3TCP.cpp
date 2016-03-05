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

#include "Zusi3TCP.h"

#include <cstdint>

namespace zusi
{
	void Attribute::write(Socket& sock) const
	{
		uint32_t length = data_bytes + sizeof(m_id);
		sock.WriteBytes(&length, sizeof(length));
		sock.WriteBytes(&m_id, sizeof(m_id));
		sock.WriteBytes(data, data_bytes);
	}

	bool Attribute::read(Socket& sock, uint32_t length)
	{
		data_bytes = length - sizeof(m_id);
		sock.ReadBytes(&m_id, sizeof(m_id));
		data = operator new(data_bytes);
		sock.ReadBytes(data, data_bytes);

		return true;
	}

	bool Node::write(Socket& sock) const
	{
		sock.WriteBytes(&NODE_START, sizeof(NODE_START));
		sock.WriteBytes(&m_id, sizeof(m_id));
		for (Attribute* a_p : attributes)
			a_p->write(sock);

		for (Node* n_p : nodes)
			n_p->write(sock);
		int written = sock.WriteBytes(&NODE_END, sizeof(NODE_END));

		if (written != sizeof(NODE_END))
			return false;

		return true;
	}

	bool Node::read(Socket& sock)
	{
		sock.ReadBytes(&m_id, sizeof(m_id));

		uint32_t next_length;
		while (true)
		{
			sock.ReadBytes(&next_length, sizeof(next_length));

			if (next_length == NODE_START)
			{
				Node* new_node = new Node();
				new_node->read(sock);
				nodes.push_back(new_node);
			}
			else if (next_length == NODE_END)
			{
				return true;
			}
			else
			{
				Attribute* new_attribute = new Attribute();
				new_attribute->read(sock, next_length);
				attributes.push_back(new_attribute);
			}
		}
	}
	
	bool Connection::receiveMessage(Node& dest) const
	{
		uint32_t header;
		m_socket->ReadBytes(&header, sizeof(header));
		if (header != 0)
			return false;

		return dest.read(*m_socket);
	}

	bool Connection::sendMessage(Node& src)
	{
		return src.write(*m_socket);
	}

	bool ClientConnection::connect(const std::vector<FuehrerstandData> fs_data)
	{
		//Send hello
		Node hello_message(1);

		zusi::Node* hello = new zusi::Node(1);
		hello_message.nodes.push_back(hello);

		zusi::Attribute* att = new zusi::Attribute(1);
		att->setValueUint16(2);
		hello->attributes.push_back(att);

		att = new zusi::Attribute(2);
		att->setValueUint16(2);
		hello->attributes.push_back(att);

		att = new zusi::Attribute(3);
		att->data_bytes = 8;
		att->data = new char[9]{ "Fahrpult" };
		hello->attributes.push_back(att);

		att = new zusi::Attribute(4);
		att->data_bytes = 3;
		att->data = new char[4]{ "2.0" };
		hello->attributes.push_back(att);

		hello_message.write(*m_socket);

		//Recieve ACK_HELLO
		Node hello_ack;
		receiveMessage(hello_ack);
		if (hello_ack.nodes.size() != 1 || hello_ack.nodes[0]->getId() != Cmd_ACK_HELLO)
		{
			throw std::runtime_error("Protocol error - invalid response from server");
		}

		//Send NEEDED_DATA
		Node needed_data_msg(MsgType_Fahrpult);
		Node* needed = new Node(Cmd_NEEDED_DATA);
		needed_data_msg.nodes.push_back(needed);
		Node* needed_fuehrerstand = new Node(0xA);
		needed->nodes.push_back(needed_fuehrerstand);

		for (FuehrerstandData fd_id : fs_data)
		{
			att = new Attribute(1);
			att->setValueUint16(fd_id);
			needed_fuehrerstand->attributes.push_back(att);
		}

		needed_data_msg.write(*m_socket);

		//Receive ACK_NEEDED_DATA
		Node data_ack;
		receiveMessage(data_ack);
		if (data_ack.nodes.size() != 1 || data_ack.nodes[0]->getId() != Cmd_ACK_NEEDED_DATA)
		{
			throw std::runtime_error("Protocol error - server refused data subscription");
		}

		return true;
	}

}