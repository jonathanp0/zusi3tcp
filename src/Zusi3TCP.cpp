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
#include <memory>
#include <stdexcept>

static const uint32_t NODE_START = 0;
static const uint32_t NODE_END = 0xFFFFFFFF;

namespace zusi {
namespace {
using socket = boost::asio::ip::tcp::tcp::socket;
static std::size_t readBytes(socket &sock, void *dest, std::size_t bytes) {
  boost::system::error_code error;
  std::size_t len = 0;
  do {
    /* TODO: Handle error */
    len += sock.read_some(
        boost::asio::buffer(reinterpret_cast<char *>(dest) + len, bytes - len),
        error);
  } while (bytes > len);
  return len;
}
static std::size_t writeBytes(socket &sock, const void *src,
                              std::size_t bytes) {
  return boost::asio::write(
      sock, boost::asio::buffer(src, bytes) /*, ignored_error*/);
}

#if __cpp_lib_make_unique
using std::make_unique;
#else
template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args &&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
#endif
}  // namespace

Node Connection::readNode() const {
  uint16_t id;
  readBytes(m_socket, &id, sizeof(id));

  Node result{id};

  uint32_t next_length;
  while (true) {
    readBytes(m_socket, &next_length, sizeof(next_length));

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
  readBytes(m_socket, &id, sizeof(id));

  std::unique_ptr<uint8_t[]> data{new uint8_t[data_bytes]};
  readBytes(m_socket, data.get(), data_bytes);

  Attribute att{id};
  att.setValueRaw(data.get(), data_bytes);

  return att;
}

Node Connection::readNodeWithHeader() const {
  uint32_t header;
  readBytes(m_socket, &header, sizeof(header));
  if (header != 0) {
    throw std::domain_error("Error parsing network message header");
  }

  return readNode();
}

std::unique_ptr<BaseMessage> Connection::receiveMessage() const {
  auto root{readNodeWithHeader()};
  if (root.getId() != zusi::MsgType_Fahrpult) {
    throw std::domain_error(
        "Application type reported by server is not Fahrpult");
  }

  switch (root.nodes[0].getId()) {
    case static_cast<uint16_t>(zusi::Command::DATA_FTD):
      return make_unique<FtdDataMessage>(root.nodes[0]);
    case static_cast<uint16_t>(zusi::Command::DATA_OPERATION):
      return make_unique<OperationDataMessage>(root.nodes[0]);
    case static_cast<uint16_t>(zusi::Command::DATA_PROG):
      return make_unique<ProgDataMessage>(root.nodes[0]);
    default:
      throw std::domain_error("Invalid command");
  }
}

bool Connection::writeNode(const Node &node) const {
  auto id = node.getId();

  writeBytes(m_socket, &NODE_START, sizeof(NODE_START));
  writeBytes(m_socket, &id, sizeof(id));
  for (const auto &a_p : node.attributes) {
    writeAttribute(a_p);
  }

  for (const Node &n_p : node.nodes) {
    writeNode(n_p);
  }
  size_t written = writeBytes(m_socket, &NODE_END, sizeof(NODE_END));

  return (written == sizeof(NODE_END));
}

void Connection::writeAttribute(const Attribute &att) const {
  auto id = att.getId();
  uint32_t length = static_cast<uint32_t>(att.getValueLen()) + sizeof(id);
  writeBytes(m_socket, &length, sizeof(length));
  writeBytes(m_socket, &id, sizeof(id));
  writeBytes(m_socket, att.getValueRaw(), att.getValueLen());
}

bool Connection::connect(const std::string &client_id,
                         const std::vector<FuehrerstandData> &fs_data,
                         const std::vector<uint16_t> &prog_data,
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
  Node hello_ack{readNodeWithHeader()};
  if (hello_ack.nodes.size() != 1 ||
      hello_ack.nodes[0].getId() !=
          static_cast<uint16_t>(
              Command::ACK_HELLO)) /* TODO: Refactor with C++17 */ {
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

  if (bedienung) {
    needed.nodes.emplace_back(Node{0xB});
  }

  if (!prog_data.empty()) {
    Node needed_prog{0xC};
    for (uint16_t prog_id : prog_data) {
      needed_prog.attributes.emplace_back(
          Attribute{1, uint16_t{static_cast<uint16_t>(prog_id)}});
    }
    needed.nodes.push_back(needed_prog);
  }

  needed_data_msg.nodes.push_back(needed);

  writeNode(needed_data_msg);

  // Receive ACK_NEEDED_DATA
  Node data_ack{readNodeWithHeader()};
  if (data_ack.nodes.size() != 1 ||
      data_ack.nodes[0].getId() !=
          static_cast<uint16_t>(
              Command::ACK_NEEDED_DATA)) /* TODO: Refactor with C++17 */ {
    throw std::runtime_error(
        "Protocol error - server refused data subscription");
  }

  return true;
}

bool Connection::sendInput(const In::Taster &taster,
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

  return writeNode(input_message);
}
}  // namespace zusi
