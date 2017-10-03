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

#include "serverconnection.h"

namespace zusi {

bool ServerConnection::accept() {
  // Recieve HELLO
  {
    Node hello_msg{readNodeWithHeader()};
    if (hello_msg.nodes.size() != 1 ||
        hello_msg.nodes[0].getId() !=
            static_cast<uint16_t>(
                Command::HELLO)) /* TODO: Refactor with C++17 */ {
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
    Node needed_data_msg{readNodeWithHeader()};
    if (needed_data_msg.nodes.size() != 1 ||
        needed_data_msg.nodes[0].getId() !=
            static_cast<uint16_t>(
                Command::NEEDED_DATA)) /* TODO: Refactor with C++17 */ {
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
        if (att.getId() != 1 || att.getValueLen() != 2) {
          continue;
        }

        uint16_t var_id = att.getValue<uint16_t>();

        if (group_id == 0xA) {
          m_fs_data.insert(static_cast<zusi::FuehrerstandData>(var_id));
        } else if (group_id == 0xC) {
          m_prog_data.insert(static_cast<zusi::ProgData>(var_id));
        }
      }
    }
  }

  // Send ACK_NEEDED_DATA
  {
    Node data_ack_message(MsgType_Fahrpult);

    zusi::Node data_ack{Command::ACK_NEEDED_DATA};
    data_ack.attributes.emplace_back(Attribute{1, uint8_t{0}});
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

  if (data.attributes.empty()) {
    return true;
  }

  return sendMessage(data_message);
}

#ifdef USE_OLD_FSDATAITEM
bool ServerConnection::sendData(std::vector<FsDataItem *> ftd_items) {
  if (ftd_items.empty()) {
    return true;
  }

  Node data_message(MsgType_Fahrpult);

  zusi::Node data{Command::DATA_FTD};

  for (const auto &item : ftd_items) {
    if (m_fs_data.count(item->getId()) == 1) {
      item->appendTo(data);
    }
  }

  data_message.nodes.push_back(data);
  return sendMessage(data_message);
}
#endif
}
