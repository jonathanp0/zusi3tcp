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

#include <cstring>
#include <iostream>

static const uint32_t NODE_START = 0;
static const uint32_t NODE_END = 0xFFFFFFFF;

static char const *NODE_START_C = "\x00\x00\x00\x00";
static char const *NODE_END_C = "\xFF\xFF\xFF\xFF";

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

MessageVariant Connection::receiveMessage() const {
  auto root{readNodeWithHeader()};
  if (root.getId() != zusi::MsgType_Fahrpult) {
    throw std::domain_error(
        "Application type reported by server is not Fahrpult");
  }

  switch (root.nodes[0].getId()) {
    case static_cast<uint16_t>(zusi::Command::DATA_FTD):
      return FtdDataMessage(root.nodes[0]);
    case static_cast<uint16_t>(zusi::Command::DATA_OPERATION):
      return OperationDataMessage(root.nodes[0]);
    case static_cast<uint16_t>(zusi::Command::DATA_PROG):
      return ProgDataMessage(root.nodes[0]);
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

template <typename NetworkType>
struct serializeValue {
  template <typename Attribute>
  std::string operator()(Attribute const &attrib) {
    return {reinterpret_cast<char const *>(&*attrib), sizeof(NetworkType)};
  }
};

template <>
struct serializeValue<void *> {
  template <typename Attribute>
  std::string operator()(Attribute const &attrib) {
    return *attrib;
  }
};

template <typename Attribute>
std::string serializeAttribute(const Attribute &attrib) {
  uint16_t id = Attribute::id;
  std::string retval;

  auto value = serializeValue<typename Attribute::networktype>{}(attrib);

  retval.reserve(3 * sizeof(uint32_t));
  uint32_t length = static_cast<uint32_t>(value.length() + sizeof(id));
  retval.append(reinterpret_cast<char const *>(&length), sizeof(length));
  retval.append(reinterpret_cast<char const *>(&id), sizeof(id));
  retval.append(value);

  return retval;
}

template <typename... ChildNodes, typename... Attribute>
auto serializeNode(Command command, std::tuple<Attribute...> attrib,
                   ChildNodes... nodes) {
  std::string retval;
  retval.reserve(4 + sizeof(command) +
                 sizeof...(Attribute) * (3 * sizeof(uint32_t)) + 4);

  retval.append(NODE_START_C, 4);
  retval.append(reinterpret_cast<char const *>(&command), sizeof(command));
  (retval.append(serializeAttribute(std::get<Attribute>(attrib))), ...);
  (retval.append(serializeNode2(nodes)), ...);
  retval.append(NODE_END_C, 4);
  return retval;
}

template <typename... Attribute>
auto serializeNode2(
    std::tuple<Command, std::tuple<Attribute...>, std::tuple<>> attrib) {
  return serializeNode(std::get<0>(attrib), std::get<1>(attrib));
}

template <typename... ChildNodes, typename... Attribute>
auto serializeNode2(
    std::tuple<Command, std::tuple<Attribute...>, std::tuple<ChildNodes...>>
        attrib) {
  return serializeNode(std::get<0>(attrib), std::get<1>(attrib),
                       std::get<2>(attrib));
}

class reader {
#pragma pack(push, 1)
  struct header_t {
    uint32_t length;
    uint16_t type;
  };
#pragma pack(pop)

 public:
  explicit reader(socket &sock) : m_sock{sock} {}

  template <typename... Ts>
  auto read(Command command) {
    header_t header;
    readBytes(m_sock, &header, sizeof(header));
    readBytes(m_sock, &header, sizeof(header));
    if (header.length != 0 || header.type != static_cast<uint16_t>(command)) {
      throw std::domain_error("Error parsing network message header");
    }

    std::tuple<Ts...> retval;
    do {
      readBytes(m_sock, &header.length, sizeof(header.length));
      if (header.length == -1u) {
        break;
      }
      readBytes(m_sock, &header.type, sizeof(header.type));

      (read_if_match(std::get<Ts>(retval), header) || ...) ||
          read_null(header.length);
    } while (true);

    readBytes(m_sock, &header.length, sizeof(header.length));
    return retval;
  }

 private:
  template <typename T>
  bool read_if_match(T &t, header_t &hdr) {
    if (T::id != hdr.type) {
      return false;
    }

    uint32_t data_bytes = hdr.length - 2;
    std::unique_ptr<uint8_t[]> data{new uint8_t[data_bytes]};
    readBytes(m_sock, data.get(), data_bytes);

    t = T{std::move(data), data_bytes};
    return true;
  }

  bool read_null(uint32_t length) {
    uint32_t data_bytes = length - 2;
    std::unique_ptr<uint8_t[]> data{new uint8_t[data_bytes]};
    readBytes(m_sock, data.get(), data_bytes);
    return true;
  }

  socket &m_sock;
};

Connection::Connection(socket &socket, const std::string &client_id)
    : m_socket(socket) {
  std::string msg = serializeNode(
      Command::HELLO, std::make_tuple(),
      std::make_tuple(
          Command::HELLO,
          std::make_tuple(Hello::ProtokollVersion{2}, ClientTyp::Fahrpult,
                          Hello::ClientId{client_id},
                          Hello::ClientVersion{"2.0"}),
          std::make_tuple()));

  boost::asio::write(m_socket, boost::asio::buffer(msg));

  uint8_t ok;
  reader rdr{m_socket};
  std::tie(m_zusiVersion, m_connectionInfo, ok) =
      rdr.read<HelloAck::ZusiVersion, HelloAck::ZusiVerbindungsinfo,
               HelloAck::ZusiOk>(Command::ACK_HELLO);

  if (ok) {
    throw std::domain_error("Zusi couldn't register our client");
  }
}

bool Connection::connect(const std::vector<FuehrerstandData> &fs_data,
                         const std::vector<uint16_t> &prog_data,
                         bool bedienung) {
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
  uint8_t ok;
  reader rdr{m_socket};
  std::tie(ok) = rdr.read<NeedDataAck::ZusiOk>(Command::ACK_NEEDED_DATA);
  if (ok) {
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
