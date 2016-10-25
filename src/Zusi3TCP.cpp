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

#include "Zusi3TCP.h"

#include <cstdint>
#include <cstring>

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

	bool ClientConnection::connect(const char* client_id, const std::vector<FuehrerstandData>& fs_data, const std::vector<ProgData>& prog_data, bool bedienung)
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
		att->data_bytes = strlen(client_id);
		att->data = new char[att->data_bytes];
		memcpy(att->data, client_id, att->data_bytes);
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
		else
		{
			for (Attribute* att : hello_ack.nodes[0]->attributes)
			{
				switch (att->getId())
				{
				case 1:
					m_zusiVersion = std::string(static_cast<char*>(att->data), att->data_bytes);
					break;
				case 2:
					m_connectionInfo = std::string(static_cast<char*>(att->data), att->data_bytes);
					break;
				default:
					break;
				}
			}
		}

		//Send NEEDED_DATA
		Node needed_data_msg(MsgType_Fahrpult);
		Node* needed = new Node(Cmd_NEEDED_DATA);
		needed_data_msg.nodes.push_back(needed);

		if (!fs_data.empty())
		{
			Node* needed_fuehrerstand = new Node(0xA);
			needed->nodes.push_back(needed_fuehrerstand);
			for (FuehrerstandData fd_id : fs_data)
			{
				att = new Attribute(1);
				att->setValueUint16(fd_id);
				needed_fuehrerstand->attributes.push_back(att);
			}
		}

		if (bedienung)
			needed->nodes.push_back(new Node(0xB));

		if (!prog_data.empty())
		{
			Node* needed_prog = new Node(0xC);
			needed->nodes.push_back(needed_prog);
			for (ProgData prog_id : prog_data)
			{
				att = new Attribute(1);
				att->setValueUint16(prog_id);
				needed_prog->attributes.push_back(att);
			}
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


	bool ClientConnection::sendInput(Tastatur taster, TastaturKommand kommand, TastaturAktion aktion, int16_t position)
	{
		Node input_message(MsgType_Fahrpult);
		Node* input = new Node(Cmd_INPUT);
		input_message.nodes.push_back(input);

		Node* action = new Node(1);
		input->nodes.push_back(action);

		//Tasterzuordnung
		Attribute* att = new Attribute(1);
		att->setValueUint16(taster);
		action->attributes.push_back(att);

		//Kommand
		att = new Attribute(2);
		att->setValueUint16(kommand);
		action->attributes.push_back(att);

		//Aktion
		att = new Attribute(3);
		att->setValueUint16(aktion);
		action->attributes.push_back(att);

		//Position
		att = new Attribute(4);
		att->setValueInt16(position);
		action->attributes.push_back(att);

		//'Spezielle funktion parameter'
		att = new Attribute(5);
		float* empty_float = new float(position);
		att->data = empty_float;
		att->data_bytes = sizeof(empty_float);
		action->attributes.push_back(att);

		return sendMessage(input_message);

	}

	bool ServerConnection::accept()
	{
		//Recieve HELLO
		{
			Node hello_msg;
			receiveMessage(hello_msg);
			if (hello_msg.nodes.size() != 1 || hello_msg.nodes[0]->getId() != Cmd_HELLO)
			{
				throw std::runtime_error("Protocol error - invalid HELLO from client");
			}
			else
			{
				for (Attribute* att : hello_msg.nodes[0]->attributes)
				{
					switch (att->getId())
					{
					case 3:
						m_clientName = std::string(static_cast<char*>(att->data), att->data_bytes);
						break;
					case 4:
						m_clientVersion = std::string(static_cast<char*>(att->data), att->data_bytes);
						break;
					default:
						break;
					}
				}
			}
		}

		//Send hello ack
		{
			Node hello_ack_message(MsgType_Connecting);

			zusi::Node* hello_ack = new zusi::Node(Cmd_ACK_HELLO);
			hello_ack_message.nodes.push_back(hello_ack);

			zusi::Attribute* att = new zusi::Attribute(1);
			att->data_bytes = 9;
			att->data = new char[10]{ "3.0.1.0" };
			hello_ack->attributes.push_back(att);

			att = new zusi::Attribute(2);
			att->setValueUint8('0');
			hello_ack->attributes.push_back(att);

			att = new zusi::Attribute(3);
			att->setValueUint8(0);
			hello_ack->attributes.push_back(att);

			sendMessage(hello_ack_message);
		}

		//Receive NEEDED_DATA
		{
			Node needed_data_msg;
			receiveMessage(needed_data_msg);
			if (needed_data_msg.nodes.size() != 1 || needed_data_msg.nodes[0]->getId() != Cmd_NEEDED_DATA)
			{
				throw std::runtime_error("Protocol error - invalid NEEDED_DATA from client");
			}

			for (const zusi::Node* node : needed_data_msg.nodes[0]->nodes)
			{
				uint16_t group_id = node->getId();

				if (group_id == 0xB)
				{
					m_bedienung = true;
					continue;
				}

				for (const Attribute* att : node->attributes)
				{
					if (att->getId() != 1 || att->data_bytes != 2)
						continue;

					uint16_t var_id = *(reinterpret_cast<uint16_t*>(att->data));

					if (group_id == 0xA)
						m_fs_data.insert(static_cast<zusi::FuehrerstandData>(var_id));
					else if (group_id == 0xC)
						m_prog_data.insert(static_cast<zusi::ProgData>(var_id));
				}

			}
		}

		//Send ACK_NEEDED_DATA
		{
			Node data_ack_message(MsgType_Fahrpult);

			zusi::Node* data_ack = new zusi::Node(Cmd_ACK_NEEDED_DATA);
			data_ack_message.nodes.push_back(data_ack);

			zusi::Attribute* att = new zusi::Attribute(1);
			att->setValueUint8(0);
			data_ack->attributes.push_back(att);

			sendMessage(data_ack_message);
		}

		
		return true;
	}

	bool ServerConnection::sendData(std::vector<std::pair<FuehrerstandData, float>> ftd_items)
	{
		Node data_message(MsgType_Fahrpult);

		zusi::Node* data = new zusi::Node(Cmd_DATA_FTD);
		data_message.nodes.push_back(data);

		for (const auto& ftd : ftd_items)
		{
			if (m_fs_data.count(ftd.first) == 1)
			{
				Attribute* att = new zusi::Attribute(ftd.first);
				att->setValueFloat(ftd.second);
				data->attributes.push_back(att);
			}
		}

		if (data->attributes.empty())
			return true;
			
		return sendMessage(data_message);

	}

	bool ServerConnection::sendData(std::vector<FsDataItem*> ftd_items)
	{
		if (ftd_items.empty())
			return true;

		Node data_message(MsgType_Fahrpult);

		zusi::Node* data = new zusi::Node(Cmd_DATA_FTD);
		data_message.nodes.push_back(data);

		for (const auto& item : ftd_items)
			if(m_fs_data.count(item->getId()) == 1)
				item->appendTo(*data);

		return sendMessage(data_message);

	}


}