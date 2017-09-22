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
#include <stdexcept>

static const uint32_t NODE_START = 0;
static const uint32_t NODE_END = 0xFFFFFFFF;

namespace zusi {
Node Connection::readNode() const {
  uint16_t id;
  m_socket->ReadBytes(&id, sizeof(id));

  Node result{id};

  uint32_t next_length;
  while (true) {
    m_socket->ReadBytes(&next_length, sizeof(next_length));

    if (next_length == NODE_START) {
      result.nodes.push_back(readNode());
    } else if (next_length == NODE_END) {
      break;
    } else {
      result.attributes.emplace_back(readAttribute(next_length));
    }
  }

  return result;
}

Attribute Connection::readAttribute(uint32_t length) const {
  uint16_t id;
  uint32_t data_bytes = length - sizeof(id);
  m_socket->ReadBytes(&id, sizeof(id));

  uint8_t data[data_bytes];
  m_socket->ReadBytes(data, data_bytes);

  Attribute att{id};
  att.setValueRaw(data, data_bytes);

  return att;
}

Node Connection::receiveMessage() const {
  uint32_t header;
  m_socket->ReadBytes(&header, sizeof(header));
  if (header != 0)
    throw std::domain_error("Error parsing network message header");

  return readNode();
}

bool Connection::writeNode(const Node &node) const {
  auto id = node.getId();

  m_socket->WriteBytes(&NODE_START, sizeof(NODE_START));
  m_socket->WriteBytes(&id, sizeof(id));
  for (const auto &a_p : node.attributes) writeAttribute(a_p);

  for (const Node &n_p : node.nodes) writeNode(n_p);
  int written = m_socket->WriteBytes(&NODE_END, sizeof(NODE_END));

  if (written != sizeof(NODE_END)) return false;

  return true;
}

void Connection::writeAttribute(const Attribute &att) const {
  auto id = att.getId();
  uint32_t length = att.getValueLen() + sizeof(id);
  m_socket->WriteBytes(&length, sizeof(length));
  m_socket->WriteBytes(&id, sizeof(id));
  m_socket->WriteBytes(att.getValueRaw(), att.getValueLen());
}

bool Connection::sendMessage(const Node &src) { return writeNode(src); }

bool ClientConnection::connect(const std::string client_id,
                               const std::vector<FuehrerstandData> &fs_data,
                               const std::vector<ProgData> &prog_data,
                               bool bedienung) {
  // Send hello
  Node hello_message(1);

  zusi::Node hello{1};

  hello.attributes.emplace_back(Hello::ProtokollVersion{2}.att());
  hello.attributes.emplace_back(ClientTyp::Fahrpult.att());
  hello.attributes.emplace_back(Hello::ClientId{client_id}.att());
  hello.attributes.emplace_back(Hello::ClientVersion{"2.0"}.att());

  hello_message.nodes.push_back(hello);
  writeNode(hello_message);

  // Recieve ACK_HELLO
  Node hello_ack{receiveMessage()};
  if (hello_ack.nodes.size() != 1 ||
      hello_ack.nodes[0].getId() != static_cast<uint16_t>(Command::ACK_HELLO)) /* TODO: Refactor with C++17 */ {
    throw std::runtime_error("Protocol error - invalid response from server");
  } else {
    for (const auto &att : hello_ack.nodes[0].attributes) {
      switch (att.getId()) {
        case HelloAck::ZusiVersion::id:
          m_zusiVersion = HelloAck::ZusiVersion{att};
          break;
        case HelloAck::ZusBerbindungsinfo::id:
          m_connectionInfo = HelloAck::ZusBerbindungsinfo{att};
          break;
        default:
          break;
      }
    }
  }

  // Send NEEDED_DATA
  Node needed_data_msg(MsgType_Fahrpult);
  Node needed{Command::NEEDED_DATA};

  if (!fs_data.empty()) {
    Node needed_fuehrerstand{0xA};
    for (FuehrerstandData fd_id : fs_data) {
      needed_fuehrerstand.attributes.emplace_back(
          Attribute{1, uint16_t{fd_id}});
    }
    needed.nodes.push_back(needed_fuehrerstand);
  }

  if (bedienung) needed.nodes.emplace_back(Node{0xB});

  if (!prog_data.empty()) {
    Node needed_prog{0xC};
    for (ProgData prog_id : prog_data) {
      needed_prog.attributes.emplace_back(Attribute{1, uint16_t{static_cast<uint16_t>(prog_id)}});
    }
    needed.nodes.push_back(needed_prog);
  }

  needed_data_msg.nodes.push_back(needed);

  writeNode(needed_data_msg);

  // Receive ACK_NEEDED_DATA
  Node data_ack{receiveMessage()};
  if (data_ack.nodes.size() != 1 ||
      data_ack.nodes[0].getId() != static_cast<uint16_t>(Command::ACK_NEEDED_DATA)) /* TODO: Refactor with C++17 */ {
    throw std::runtime_error(
        "Protocol error - server refused data subscription");
  }

  return true;
}

bool ClientConnection::sendInput(const In::Taster &taster,
                                 const In::Kommando &kommando,
                                 const In::Aktion &aktion, uint16_t position) {
  Node input_message(MsgType_Fahrpult);
  Node input{Command::INPUT};

  Node action{1};

  action.attributes.emplace_back(taster.att());
  action.attributes.emplace_back(kommando.att());
  action.attributes.emplace_back(aktion.att());
  action.attributes.emplace_back(In::Position{position}.att());
  action.attributes.emplace_back(In::Spezial{.0}.att());

  input.nodes.push_back(action);
  input_message.nodes.push_back(input);

  return sendMessage(input_message);
}

bool ServerConnection::accept() {
  // Recieve HELLO
  {
    Node hello_msg{receiveMessage()};
    if (hello_msg.nodes.size() != 1 ||
        hello_msg.nodes[0].getId() != static_cast<uint16_t>(Command::HELLO)) /* TODO: Refactor with C++17 */ {
      throw std::runtime_error("Protocol error - invalid HELLO from client");
    } else {
      for (const Attribute &att : hello_msg.nodes[0].attributes) {
        size_t len;
        const char *data =
            reinterpret_cast<const char *>(att.getValueRaw(&len));
        switch (att.getId()) {
          case 3:
            m_clientName = std::string(data, len);
            break;
          case 4:
            m_clientVersion = std::string(data, len);
            break;
          default:
            break;
        }
      }
    }
  }

  // Send hello ack
  {
    Node hello_ack_message(MsgType_Connecting);

    zusi::Node hello_ack{Command::ACK_HELLO};

    zusi::Attribute attVersion{1};
    attVersion.setValueRaw(reinterpret_cast<const uint8_t *>("3.1.2.0"),
                           sizeof("3.1.2.0"));
    hello_ack.attributes.push_back(attVersion);

    hello_ack.attributes.emplace_back(Attribute{2, uint8_t{'0'}});

    hello_ack.attributes.emplace_back(Attribute{3, uint8_t{0}});

    hello_ack_message.nodes.push_back(hello_ack);

    sendMessage(hello_ack_message);
  }

  // Receive NEEDED_DATA
  {
    Node needed_data_msg{receiveMessage()};
    if (needed_data_msg.nodes.size() != 1 ||
        needed_data_msg.nodes[0].getId() != static_cast<uint16_t>(Command::NEEDED_DATA)) /* TODO: Refactor with C++17 */ {
      throw std::runtime_error(
          "Protocol error - invalid NEEDED_DATA from client");
    }

    for (const zusi::Node &node : needed_data_msg.nodes[0].nodes) {
      uint16_t group_id = node.getId();

      if (group_id == 0xB) {
        m_bedienung = true;
        continue;
      }

      for (const Attribute &att : node.attributes) {
        if (att.getId() != 1 || att.getValueLen() != 2) continue;

        uint16_t var_id = att.getValue<uint16_t>();

        if (group_id == 0xA)
          m_fs_data.insert(static_cast<zusi::FuehrerstandData>(var_id));
        else if (group_id == 0xC)
          m_prog_data.insert(static_cast<zusi::ProgData>(var_id));
      }
    }
  }

  // Send ACK_NEEDED_DATA
  {
    Node data_ack_message(MsgType_Fahrpult);

    zusi::Node data_ack{Command::ACK_NEEDED_DATA};
    data_ack.attributes.push_back(Attribute{1, uint8_t{0}});
    data_ack_message.nodes.push_back(data_ack);

    sendMessage(data_ack_message);
  }

  return true;
}

bool ServerConnection::sendData(
    std::vector<std::pair<FuehrerstandData, float>> ftd_items) {
  Node data_message(MsgType_Fahrpult);

  zusi::Node data{Command::DATA_FTD};

  for (const auto &ftd : ftd_items) {
    if (m_fs_data.count(ftd.first) == 1) {
      data.attributes.emplace_back(Attribute{ftd.first, float{ftd.second}});
    }
  }

  data_message.nodes.push_back(data);

  if (data.attributes.empty()) return true;

  return sendMessage(data_message);
}

bool ServerConnection::sendData(std::vector<FsDataItem *> ftd_items) {
  if (ftd_items.empty()) return true;

  Node data_message(MsgType_Fahrpult);

  zusi::Node data{Command::DATA_FTD};

  for (const auto &item : ftd_items)
    if (m_fs_data.count(item->getId()) == 1) item->appendTo(data);

  data_message.nodes.push_back(data);
  return sendMessage(data_message);
}
}
