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
/** @file */

#pragma once

#include <array>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "string.h"  // For memcpy

#include <chrono>

//! Zusi Namespace
namespace zusi {
//! Message Type Node ID - used for root node of message
enum MsgType { MsgType_Connecting = 1, MsgType_Fahrpult = 2 };

//! Command Node ID - type of sub-nodes of root node
enum Command {
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

using FuehrerstandData = uint16_t;
/*
 TODO - maybe we want to keep this enum, while then we have the name in two
palces
//! Fuehrerstand Data variable ID's
enum FuehrerstandData
{
        Fs_Geschwindigkeit = 1,
        Fs_DruckHauptlufleitung = 2,
        Fs_DruckBremszylinder = 3,
        Fs_DruckHauptluftbehaelter = 4,
        Fs_Oberstrom = 13,
        Fs_Fahrleitungsspannung = 14,
        Fs_Motordrehzahl = 15,
        Fs_UhrzeitStunde = 16,
        Fs_UhrzeitMinute = 17,
        Fs_UhrzeitSekunde = 18,
        Fs_Hauptschalter = 19,
        Fs_AfbSollGeschwindigkeit = 23,
Fs_Gesamtweg = 25,
        Fs_UhrzeitDigital = 35,
        Fs_AfbEinAus = 54,
        Fs_Datum = 75,
        Fs_Sifa = 100
};
*/

//! Program status Data variable ID's
enum ProgData {
  Prog_Zugdatei = 1,
  Prog_Zugnummer,
  Prog_SimStart,
  Prog_BuchfahrplanDatei
};

//! Abstract interface for a socket
class Socket {
 public:
  virtual ~Socket() {}

  /** @brief Try to read bytes into dest from socket.
  *
  * Method should block until all requested bytes are read, or the stream ends
  * @param dest Data buffer to write to
  * @param bytes Number of bytes to read
  * @return Number of bytes read
  */
  virtual int ReadBytes(void* dest, int bytes) = 0;

  /**
  * @brief Try to write bytes from src to socket.
  * @param src Data buffer to read from
  * @param bytes Number of bytes to read
  * @return Number of bytes successfully written
  */
  virtual int WriteBytes(const void* src, int bytes) = 0;

  /**
  * @brief Check if there is incoming data waiting to be read
  * @return True if data available, otherwise false
  */
  virtual bool DataToRead() = 0;
};

/**
* @brief Generic Zusi message attribute.
* Data is owned by the class
*/
class Attribute {
 public:
  //! Construct an empty attribute
  Attribute() : data_bytes(0), data{0}, m_id(0) {}

  /** @brief Constructs an attribute
  * @param id Attribute ID
  */
  explicit constexpr Attribute(uint16_t id)
      : data_bytes(0), data{0}, m_id(id) {}

  template <typename T>
  Attribute(uint16_t id, T value) : data_bytes(0), data{0}, m_id(id) {
    setValue<T>(value);
  }

  Attribute(const Attribute&) = default;
  Attribute(Attribute&&) = default;
  Attribute& operator=(const Attribute& o) = default;

  //! Get Attribute ID
  constexpr uint16_t getId() const { return m_id; }

  //! Utility function to set the value
  template <typename T>
  void setValue(T value) {
    static_assert(sizeof(T) <= sizeof(data),
                  "Trying to write too big data into attribute");
    *reinterpret_cast<T*>(data) = value;
    data_bytes = sizeof(T);
  }

  void setValueRaw(const uint8_t* data, size_t length) {
    if (length > sizeof(this->data)) {
      throw std::runtime_error("Trying to write too much data");
    }
    data_bytes = length;
    memcpy(this->data, data, length);
  }

  bool hasValue() const { return data_bytes != 0; }

  template <typename T>
  bool hasValueType() const {
    return sizeof(T) == data_bytes;
  }

  size_t getValueLen() const { return data_bytes; }

  template <typename T>
  T getValue() const {
    static_assert(sizeof(T) <= sizeof(data),
                  "Trying to read too big data into attribute");
    return *reinterpret_cast<const T*>(data);
  }

  const uint8_t* getValueRaw(size_t* len = nullptr) const {
    if (len) {
      *len = data_bytes;
    }
    return data;
  }

 private:
  //! Number of bytes in #data
  uint32_t data_bytes;

  //! Attribute data
  uint8_t data[255];  // more or less random length, this won't be enough for
                      // the timetable XML, but most likely this client
  // won't read such large messages correctly either due to ignoring
  // read()/recv()'s return value and
  // network fragmentation

  uint16_t m_id;
};

template <uint16_t id_, typename NetworkType, typename CPPType = NetworkType>
struct AttribTag {
  using networktype = NetworkType;
  using type = CPPType;
  constexpr static auto id = id_;
  type value;

  constexpr AttribTag() {}

  constexpr AttribTag(type val) : value{val} {}

  AttribTag(const Attribute& attrib)
      : value{static_cast<type>(attrib.getValue<networktype>())} {
    if (id != attrib.getId()) {
      throw std::runtime_error{"Invalid conversion of attribute"};
    }
  }

  type& operator*() { return value; }

  constexpr operator type() const { return value; }

  constexpr bool operator==(
      const AttribTag<id_, NetworkType, CPPType>& o) const {
    return o.value == value;
  }

  Attribute att() const {
    return Attribute{id, static_cast<NetworkType>(value)};
  }

  operator Attribute() const { return att(); }
};

template <uint16_t id_>
struct AttribTag<id_, std::string, std::string>
    : public AttribTag<id_, void*, std::string> {
  using parentT = AttribTag<id_, void*, std::string>;

  AttribTag(const Attribute& attrib)
      : parentT{std::string{reinterpret_cast<const char*>(attrib.getValueRaw()),
                            attrib.getValueLen()}} {
    if (id_ != attrib.getId()) {
      throw std::runtime_error{"Invalid conversion of attribute"};
    }
  }

  AttribTag(const std::string& str) : parentT{str} {}

  Attribute att() const {
    Attribute att{id_};
    att.setValueRaw(reinterpret_cast<const uint8_t*>(parentT::value.c_str()),
                    parentT::value.size());
    return att;
  }

  operator Attribute() const { return att(); }
};

namespace Hello {
using ProtokollVersion = AttribTag<1, uint16_t>;
using ClientTyp = AttribTag<2, uint16_t>;
using ClientId = AttribTag<3, std::string>;
using ClientVersion = AttribTag<4, std::string>;
}

namespace ClientTyp {
constexpr Hello::ClientTyp Zusi{1};
constexpr Hello::ClientTyp Fahrpult{2};
}

namespace HelloAck {
using ZusiVersion = AttribTag<1, std::string>;
using ZusBerbindungsinfo = AttribTag<2, std::string>;
}

namespace FS {
using Geschwindigkeit = AttribTag<1, float>;
using DruckHauptlufleitung = AttribTag<2, float>;
using DruckBremszylinder = AttribTag<3, float>;
using DruckHauptluftbehaelter = AttribTag<4, float>;
using LuftpresserLaeuft = AttribTag<5, float, bool>;
using Oberstrom = AttribTag<13, float>;
using Fahrleitungsspannung = AttribTag<14, float>;
using Motordrehzahl = AttribTag<15, float>;
using UhrzeitStunde = AttribTag<16, float, int>;
using UhrzeitStundeF = AttribTag<16, float>;
using UhrzeitMinute = AttribTag<17, float, int>;
using UhrzeitMinuteF = AttribTag<17, float>;
using UhrzeitSekunde = AttribTag<18, float, int>;
using UhrzeitSekundeF = AttribTag<18, float>;
using Hauptschalter = AttribTag<19, float, bool>;
using AfbSollGeschwindigkeit = AttribTag<23, float>;
using Gesamtweg = AttribTag<25, float>;
using LMSchleudern = AttribTag<27, float, bool>;
using UhrzeitDigital = AttribTag<35, float>;
using AfbEinAus = AttribTag<54, float>;
using Datum = AttribTag<75, float>;
using Streckenhoechstgeschwindigkeit = AttribTag<77, float>;
using Sifa = AttribTag<100, float>;
}

namespace In {
using Taster = AttribTag<1, uint16_t>;
using Kommando = AttribTag<2, uint16_t>;
using Aktion = AttribTag<3, uint16_t>;
using Position = AttribTag<4, uint16_t>;
using Spezial = AttribTag<5, float>;
}

namespace Taster {
constexpr In::Taster Fahrschalter{1};
constexpr In::Taster DynBremse{2};
constexpr In::Taster Pfeife{0x0C};
constexpr In::Taster Sifa{0x10};
}

namespace Kommando {
constexpr In::Kommando Unbestimmt{0};
constexpr In::Kommando FahrschalterAuf_Down{1};
constexpr In::Kommando FahrschalterAuf_Up{2};
constexpr In::Kommando FahrschalterAb_Down{3};
constexpr In::Kommando FahrschalterAb_Up{4};
constexpr In::Kommando SifaDown{0x39};
constexpr In::Kommando SifaUp{0x3A};
constexpr In::Kommando PfeifeDown{0x45};
constexpr In::Kommando PfeifeUp{0x46};
}

namespace Aktion {
constexpr In::Aktion Default{0};
constexpr In::Aktion Down{1};
constexpr In::Aktion Up{2};
constexpr In::Aktion AufDown{3};
constexpr In::Aktion AufUp{4};
constexpr In::Aktion AbDown{5};
constexpr In::Aktion AbUp{6};
constexpr In::Aktion Absolut{7};
constexpr In::Aktion Absolut1000er{8};
}

//! Generic Zusi message node
class Node {
 public:
  //! Constructs an empty node
  explicit Node() : m_id(0) {}

  /** @brief Constructs an Node
  * @param id Attribute ID
  */
  explicit Node(uint16_t id) : m_id(id) {}

  Node(const Node& o) = default;
  Node(Node&&) = default;
  Node& operator=(const Node& o) = default;

  //! Get Attribute ID
  uint16_t getId() const { return m_id; }

  //!  Attributes of this node
  std::vector<Attribute> attributes;
  //!  Sub-nodes of this node
  std::vector<Node> nodes;

 private:
  uint16_t m_id;

  static const uint32_t NODE_START = 0;
  static const uint32_t NODE_END = 0xFFFFFFFF;
};

/**
@brief Represents an piece of data which is sent as part of DATA_FTD message

It should be comparable so that the application can check if the data has
changed
before transmitting.
*/
class FsDataItem {
 public:
  FsDataItem(FuehrerstandData id) : m_id(id) {}

  //! Append this data item to the specified node
  virtual void appendTo(Node& node) const = 0;
  virtual bool operator==(const FsDataItem& a) = 0;

  bool operator!=(const FsDataItem& a) { return !(*this == a); }

  //! Returns the Fuehrerstand Data type ID of this data item
  FuehrerstandData getId() const { return m_id; }

 private:
  FuehrerstandData m_id;
};

//! Generic class for a float DATA_FTD item
class FloatFsDataItem : public FsDataItem {
 public:
  FloatFsDataItem(FuehrerstandData id, float value)
      : FsDataItem(id), m_value(value) {}

  virtual bool operator==(const FsDataItem& a) {
    if (getId() != a.getId()) return false;

    const FloatFsDataItem* pA = dynamic_cast<const FloatFsDataItem*>(&a);
    if (pA && pA->m_value == m_value) return true;

    return false;
  }

  virtual void appendTo(Node& node) const {
    Attribute att{getId()};
    att.setValue<float>(m_value);
    node.attributes.push_back(att);
  }

  float m_value;
};

//! Special class for the DATA_FTD structure for SIFA information
class SifaFsDataItem : public FsDataItem {
 public:
  SifaFsDataItem(bool haupt, bool licht)
      : FsDataItem(FS::Sifa::id), m_licht(licht), m_hauptschalter(haupt) {}

  virtual bool operator==(const FsDataItem& a) {
    if (getId() != a.getId()) return false;

    const SifaFsDataItem* pA = dynamic_cast<const SifaFsDataItem*>(&a);
    if (pA && pA->m_licht == m_licht &&
        pA->m_hauptschalter == m_hauptschalter &&
        pA->m_hupewarning == m_hupewarning && pA->m_hupebrems == m_hupebrems &&
        pA->m_storschalter == m_storschalter &&
        pA->m_luftabsper == m_luftabsper)
      return true;

    return false;
  }

  virtual void appendTo(Node& node) const {
    Node sNode{getId()};

    sNode.attributes.emplace_back(Attribute{1, uint8_t{'0'}});
    sNode.attributes.emplace_back(Attribute{2, uint8_t{m_licht}});
    sNode.attributes.emplace_back(Attribute{
        3, m_hupebrems ? uint8_t{2} : m_hupewarning ? uint8_t{1} : uint8_t{0}});
    sNode.attributes.emplace_back(
        Attribute{4, m_hauptschalter ? uint8_t{2} : uint8_t{1}});
    sNode.attributes.emplace_back(
        Attribute{5, m_storschalter ? uint8_t{2} : uint8_t{1}});
    sNode.attributes.emplace_back(
        Attribute{6, m_luftabsper ? uint8_t{2} : uint8_t{1}});

    node.nodes.push_back(sNode);
  }

  bool m_licht = false, m_hupewarning = false, m_hupebrems = false,
       m_hauptschalter = true, m_storschalter = true, m_luftabsper = true;
};

//! Parent class for a connection
class Connection {
 public:
  /**
  * @brief Create connection which will communicate over socket
  * @param socket The socket - class does not take ownership of it
  */
  Connection(Socket* socket) : m_socket(socket) {}

  virtual ~Connection() {}

  //! Receive a message
  Node receiveMessage() const;

  //! Send a message
  bool sendMessage(const Node& src);

  //! Check if there is data read
  bool dataAvailable() { return m_socket->DataToRead(); }

 private:
  Node readNode() const;
  Attribute readAttribute(uint32_t length) const;

  void writeAttribute(const Attribute& attr) const;

 protected:
  // TODO: ClientConnecton's calls to this via sendMessage()?
  bool writeNode(const Node& sock) const;

 protected:
  Socket* m_socket;
};

//! Manages connection to a Zusi server
class ClientConnection : public Connection {
 public:
  ClientConnection(Socket* socket) : Connection(socket) {}

  virtual ~ClientConnection() {}

  /**
  * @brief Set up connection to server
  *
  * Sends the HELLO and NEEDED_DATA commands and processes the results
  *
  * @param client_id Null-terminated character array with an identification
  * string for the client
  * @param fs_data Fuehrerstand Data ID's to subscribe to
  * @param prog_data Zusi program status ID's to subscribe to
  * @param bedienung Subscribe to input events if true
  * @return True on success
  */
  bool connect(const std::string client_id,
               const std::vector<FuehrerstandData>& fs_data,
               const std::vector<ProgData>& prog_data, bool bedienung);

  /**
  * @brief Send an INPUT command
  * @return True on success
  */
  bool sendInput(const In::Taster& taster, const In::Kommando& kommand,
                 const In::Aktion& aktion, uint16_t position);

  //! Get the version string supplied by the server
  std::string getZusiVersion() { return m_zusiVersion; }

  //! Get the connection info string supplied by the server
  std::string getConnectionnfo() { return m_connectionInfo; }

 private:
  std::string m_zusiVersion;
  std::string m_connectionInfo;
};

//! A Zusi Server emulator
class ServerConnection : public Connection {
 public:
  ServerConnection(Socket* socket) : Connection(socket), m_bedienung(false) {}

  virtual ~ServerConnection() {}

  //! Run connection handshake with client
  bool accept();

  /**
  * @brief Send FuehrerstandData updates to the client
  *
  * Only data that was requested by the client will actually be sent
  */
  bool sendData(std::vector<std::pair<FuehrerstandData, float>> ftd_items);

  /**
  * @brief Send FuehrerstandData updates to the client
  *
  * Only data that was requested by the client will actually be sent
  */
  bool sendData(std::vector<FsDataItem*> ftd_items);

  //! Get the version string supplied by the client
  std::string getClientVersion() { return m_clientVersion; }

  //! Get the connection info string supplied by the client
  std::string getClientName() { return m_clientName; }

 private:
  std::string m_clientVersion;
  std::string m_clientName;

  std::set<FuehrerstandData> m_fs_data;
  std::set<ProgData> m_prog_data;
  bool m_bedienung;
};
}
