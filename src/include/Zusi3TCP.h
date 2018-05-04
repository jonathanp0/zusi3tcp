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

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

#ifdef __has_include
#if __has_include(<experimental/optional>)
#include <experimental/optional>
#elif __has_include(<optional>)
#include <optional>
#else
#error "Missing <optional>"
#endif
#else
#error "Missing __has_include"
#endif

//! Zusi Namespace
namespace zusi {

#if __cpp_lib_experimental_optional
using std::experimental::make_optional;
using std::experimental::optional;
#elif __cpp_lib_optional
using std::make_optional;
using std::optional;
#else
#error "No usable optional"
#endif

//! Message Type Node ID - used for root node of message
enum MsgType { MsgType_Connecting = 1, MsgType_Fahrpult = 2 };

//! Command Node ID - type of sub-nodes of root node
enum class Command : uint16_t {
  HELLO = 1,
  ACK_HELLO = 2,
  NEEDED_DATA = 3,
  ACK_NEEDED_DATA = 4,
  DATA_FTD = 0xA,
  DATA_OPERATION = 0xB,
  DATA_PROG = 0xC,
  INPUT = 0x010A,
  CONTROL = 0x010B,
  GRAPHIC = 0x010C
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

//! Abstract interface for a socket
class Socket {
 public:
  virtual ~Socket() {}

  virtual bool connect() = 0;

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
  Attribute() : data{}, m_id(0) {}

  /** @brief Constructs an attribute
   * @param id Attribute ID
   */
  explicit Attribute(uint16_t id) : data{}, m_id(id) {}

  template <typename T>
  Attribute(uint16_t id, T value) : data{}, m_id(id) {
    setValue<T>(value);
  }

  Attribute(const Attribute&) = default;
  Attribute(Attribute&&) = default;
  Attribute& operator=(const Attribute& o) = default;

  //! Get Attribute ID
  uint16_t getId() const { return m_id; }

  //! Utility function to set the value
  template <typename T>
  void setValue(T value) {
    data.assign(reinterpret_cast<uint8_t*>(&value), sizeof(T));
  }

  void setValueRaw(const uint8_t* data, size_t length) {
    this->data.assign(data, length);
  }

  bool hasValue() const { return data.length() != 0; }

  template <typename T>
  bool hasValueType() const {
    return sizeof(T) == data.length();
  }

  size_t getValueLen() const { return data.length(); }

  template <typename T>
  T getValue() const {
    return *reinterpret_cast<const T*>(data.c_str());
  }

  const uint8_t* getValueRaw(size_t* len = nullptr) const {
    if (len) {
      *len = data.length();
    }
    return data.c_str();
  }

 private:
  //! Attribute data
  std::basic_string<uint8_t> data;

  uint16_t m_id;
};

template <uint16_t id_, typename NetworkType, typename CPPType = NetworkType>
struct AttribTag {
  using networktype = NetworkType;
  using type = CPPType;
  enum key { id = id_ };
  // constexpr static auto id = id_;

  constexpr AttribTag() {}

  constexpr AttribTag(type val) : value{val} {}

  AttribTag(const Attribute& attrib)
      : value{static_cast<type>(attrib.getValue<networktype>())} {
    if (id != attrib.getId()) {
      throw std::runtime_error{"Invalid conversion of attribute"};
    }
  }

  type& operator*() { return value; }
  const type& operator*() const { return value; }

  constexpr operator type() const { return value; }

  constexpr bool operator==(
      const AttribTag<id_, NetworkType, CPPType>& o) const {
    return o.value == value;
  }

  Attribute att() const {
    return Attribute{id, static_cast<NetworkType>(value)};
  }

  operator Attribute() const { return att(); }

 protected:
  type value;
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

  AttribTag() : parentT{} {}

  AttribTag(const std::string& str) : parentT{str} {}

  Attribute att() const {
    Attribute att{id_};
    att.setValueRaw(reinterpret_cast<const uint8_t*>(parentT::value.c_str()),
                    parentT::value.size());
    return att;
  }

  operator Attribute() const { return att(); }
};

//! Generic Zusi message node
class Node {
 public:
  //! Constructs an empty node
  explicit Node() : m_id(0) {}

  /** @brief Constructs an Node
   * @param id Attribute ID
   */
  explicit Node(uint16_t id) : m_id(id) {}
  explicit Node(Command cmd) : Node(static_cast<uint16_t>(cmd)) {}

  Node(const Node& o) = default;
  Node(Node&&) = default;
  Node& operator=(const Node& o) = default;

  //! Get Attribute ID
  uint16_t getId() const { return m_id; }

  template <typename T>
  typename std::enable_if<std::is_convertible<T, Attribute>::value,
                          optional<T>>::type
  get() const noexcept {
    return getImpl<T>(attributes);
  }

  template <typename T>
  typename std::enable_if<!std::is_convertible<T, Attribute>::value,
                          optional<T>>::type
  get() const noexcept {
    /* TODO: We should do a positive check here, but currently ComplexNode has
     * nothing trivially checkable without C++17 */
    return getImpl<T>(nodes);
  }

  //!  Attributes of this node
  std::vector<Attribute> attributes;
  //!  Sub-nodes of this node
  std::vector<Node> nodes;

 private:
  uint16_t m_id;

  template <typename T, typename L>
  optional<T> getImpl(const L& list) const noexcept {
    const auto& element =
        std::find_if(list.cbegin(), list.cend(),
                     [](const auto& e) { return e.getId() == T::id; });
    if (element == list.cend()) {
      return optional<T>{};
    }
    return make_optional<T>(*element);
  }
};

template <uint16_t id_, typename... Atts>
class ComplexNode {
 public:
  // constexpr static auto id = id_;
  enum vals { id = id_ };

 private:
  using AttTupleT = std::tuple<Atts...>;
  AttTupleT atts;

  template <int N>
  void matchAttribute(const Node& node, std::false_type) {}

  template <int N>
  void matchAttribute(const Node& node, std::true_type) {
    constexpr int id = std::tuple_element<N - 1, AttTupleT>::type::id;
    auto att = std::find_if(
        node.attributes.cbegin(), node.attributes.cend(),
        [id](const Attribute& att) -> bool { return att.getId() == id; });
    if (att != node.attributes.cend()) {
      std::get<N - 1>(atts) = *att;
    } else {
      throw std::runtime_error("Attribute not found");
    }
    matchAttribute<N - 1>(node, std::integral_constant<bool, (N - 1 > 0)>{});
  }

 public:
  ComplexNode() = default;

  ComplexNode(const Node& node) : atts{} {
    if (id_ != node.getId()) {
      throw std::runtime_error{"Invalid conversion of attribute"};
    }

    matchAttribute<std::tuple_size<AttTupleT>::value>(node, std::true_type{});
  }

  template <typename search>
  const search& get() const noexcept {
    return std::get<search>(atts);
  }

  template <typename search>
  search& get() noexcept {
    return std::get<search>(atts);
  }

#if __cpp_lib_apply > 0
#error Yay a new compiler \o/ we can add this feature easily!
  Node node() const {
    Node result{id_};
    std::apply(/*...*/);
    return result;
  }
#endif
};

namespace Hello {
using ProtokollVersion = AttribTag<1, uint16_t>;
using ClientTyp = AttribTag<2, uint16_t>;
using ClientId = AttribTag<3, std::string>;
using ClientVersion = AttribTag<4, std::string>;
}  // namespace Hello

namespace ClientTyp {
constexpr Hello::ClientTyp Zusi{1};
constexpr Hello::ClientTyp Fahrpult{2};
}  // namespace ClientTyp

namespace HelloAck {
using ZusiVersion = AttribTag<1, std::string>;
using ZusBerbindungsinfo = AttribTag<2, std::string>;
}  // namespace HelloAck

namespace Sifa {
/* TODO: For these types enums with the value names might be nice, or constexpr
 * instantiations as with the keyboard */
using Bauart = AttribTag<1, std::string>;
using Leuchtmelder = AttribTag<2, uint8_t>;
using Hupe = AttribTag<3, uint8_t>;
using Hauptschalter = AttribTag<4, uint8_t>;
using Stoerschalter = AttribTag<5, uint8_t>;
using Luftabsperrhahn = AttribTag<6, uint8_t>;
}  // namespace Sifa

namespace FS {
using Geschwindigkeit = AttribTag<1, float>;
using DruckHauptlufleitung = AttribTag<2, float>;
using DruckBremszylinder = AttribTag<3, float>;
using DruckHauptluftbehaelter = AttribTag<4, float>;
using LuftpresserLaeuft = AttribTag<5, float, bool>;
using ZugkraftGesamt = AttribTag<9, float>;
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
using Sifa =
    ComplexNode<100, zusi::Sifa::Bauart, zusi::Sifa::Leuchtmelder,
                zusi::Sifa::Hupe, zusi::Sifa::Hauptschalter,
                zusi::Sifa::Stoerschalter, zusi::Sifa::Luftabsperrhahn>;
}  // namespace FS

namespace ProgData {
using Zugdatei = AttribTag<1, std::string>;
using Zugnummer = AttribTag<2, std::string>;
using SimStart = AttribTag<3, float>;
using BuchfahrplanDatei = AttribTag<4, std::string>;
}  // namespace ProgData

namespace In {
using Taster = AttribTag<1, uint16_t>;
using Kommando = AttribTag<2, uint16_t>;
using Aktion = AttribTag<3, uint16_t>;
using Position = AttribTag<4, uint16_t>;
using Spezial = AttribTag<5, float>;
}  // namespace In

namespace Taster {
constexpr In::Taster KeineTastaturbedienung{0};
constexpr In::Taster Fahrschalter{1};
constexpr In::Taster DynamischeBremse{2};
constexpr In::Taster AFB{3};
constexpr In::Taster Fuehrerbremsventil{4};
constexpr In::Taster Zusatzbremsventil{5};
constexpr In::Taster Gang{6};
constexpr In::Taster Richtungsschalter{7};
constexpr In::Taster Stufenschalter{8};
constexpr In::Taster Sander{9};
constexpr In::Taster Tueren{10};
constexpr In::Taster Licht{11};
constexpr In::Taster Pfeife{12};
constexpr In::Taster Glocke{13};
constexpr In::Taster Luefter{14};
constexpr In::Taster Zugsicherung{15};
constexpr In::Taster Sifa{16};
constexpr In::Taster Hauptschalter{17};
constexpr In::Taster Gruppenschalter{18};
constexpr In::Taster Schleuderschutz{19};
constexpr In::Taster MgBremse{20};
constexpr In::Taster LokbremseEntlueften{21};
constexpr In::Taster Individuell01{22};
constexpr In::Taster Individuell02{23};
constexpr In::Taster Individuell03{24};
constexpr In::Taster Individuell04{25};
constexpr In::Taster Individuell05{26};
constexpr In::Taster Individuell06{27};
constexpr In::Taster Individuell07{28};
constexpr In::Taster Individuell08{29};
constexpr In::Taster Individuell09{30};
constexpr In::Taster Individuell10{31};
constexpr In::Taster Individuell11{32};
constexpr In::Taster Individuell12{33};
constexpr In::Taster Individuell13{34};
constexpr In::Taster Individuell14{35};
constexpr In::Taster Individuell15{36};
constexpr In::Taster Individuell16{37};
constexpr In::Taster Individuell17{38};
constexpr In::Taster Individuell18{39};
constexpr In::Taster Individuell19{40};
constexpr In::Taster Individuell20{41};
constexpr In::Taster Programmsteuerung{42};
constexpr In::Taster Stromabnehmer{43};
constexpr In::Taster Fuehrerstandssicht{44};
constexpr In::Taster LuftpresserAus{45};
constexpr In::Taster Zugfunk{46};
constexpr In::Taster LZB{47};
constexpr In::Taster Individuell21{48};
constexpr In::Taster Individuell22{49};
constexpr In::Taster Individuell23{50};
constexpr In::Taster Individuell24{51};
constexpr In::Taster Individuell25{52};
constexpr In::Taster Individuell26{53};
constexpr In::Taster Individuell27{54};
constexpr In::Taster Individuell28{55};
constexpr In::Taster Individuell29{56};
constexpr In::Taster Individuell30{57};
constexpr In::Taster Individuell31{58};
constexpr In::Taster Individuell32{59};
constexpr In::Taster Individuell33{60};
constexpr In::Taster Individuell34{61};
constexpr In::Taster Individuell35{62};
constexpr In::Taster Individuell36{63};
constexpr In::Taster Individuell37{64};
constexpr In::Taster Individuell38{65};
constexpr In::Taster Individuell39{66};
constexpr In::Taster Individuell40{67};
constexpr In::Taster Notaus{68};
constexpr In::Taster Federspeicherbremse{69};
constexpr In::Taster BatterieHauptschalterAus{70};
constexpr In::Taster NBUE{71};
constexpr In::Taster Bremsprobefunktion{72};
constexpr In::Taster LeistungAus{73};
}  // namespace Taster

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
}  // namespace Kommando

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
}  // namespace Aktion

class BaseMessage {
 protected:
  const Node root;

 public:
  BaseMessage(const Node root) : root{std::move(root)} {}
  BaseMessage(Node&& root) : root{std::move(root)} {}

  BaseMessage(const BaseMessage&) = default;
  BaseMessage(BaseMessage&&) = default;

  virtual ~BaseMessage() {}
};

class FtdDataMessage : public BaseMessage {
 public:
  FtdDataMessage(const Node root) : BaseMessage{std::move(root)} {}
  FtdDataMessage(Node&& root) : BaseMessage{root} {}

  FtdDataMessage(const FtdDataMessage&) = default;
  FtdDataMessage(FtdDataMessage&&) = default;

  template <typename T>
  optional<T> get() const noexcept {
    return root.get<T>();
  }
};

class OperationDataMessage : public BaseMessage {
 public:
  using BetaetigungT = ComplexNode<1, In::Taster, In::Kommando, In::Aktion,
                                   In::Position, In::Spezial>;
  class iterator {
    using InnerT = std::vector<Node>::const_iterator;
    InnerT inner, end;

   public:
    // TODO: When dropping the filtering in operator++() the end can be removed
    iterator(InnerT inner, InnerT end) : inner(inner), end(end) {}

    bool operator==(const iterator& other) const {
      return inner == other.inner;
    }
    bool operator!=(const iterator& other) const {
      return inner != other.inner;
    }
    iterator operator++() {
      ++inner;
      // TODO - We filter out "Kombischalter", maybe till we have std::variant?
      inner =
          std::find_if_not(inner, end, [](auto it) { return it.getId() == 2; });
      return iterator{inner, end};
    }
    BetaetigungT operator*() const { return BetaetigungT{*inner}; }
  };

  OperationDataMessage(const Node root) : BaseMessage{std::move(root)} {}
  OperationDataMessage(Node&& root) : BaseMessage{root} {}

  OperationDataMessage(const OperationDataMessage&) = default;
  OperationDataMessage(OperationDataMessage&&) = default;

  iterator begin() const noexcept {
    return iterator{std::find_if_not(root.nodes.begin(), root.nodes.end(),
                                     [](auto it) { return it.getId() == 2; }),
                    root.nodes.end()};
  }
  iterator end() const noexcept {
    return iterator{root.nodes.end(), root.nodes.end()};
  }

  decltype(root.nodes.size()) size() const noexcept {
    return root.nodes.size();
  }
};

class ProgDataMessage : public BaseMessage {
 public:
  ProgDataMessage(const Node root) : BaseMessage{std::move(root)} {}
  ProgDataMessage(Node&& root) : BaseMessage{root} {}

  ProgDataMessage(const ProgDataMessage&) = default;
  ProgDataMessage(ProgDataMessage&&) = default;

  template <typename T>
  optional<T> get() const noexcept {
    return root.get<T>();
  }
};

namespace {
template <typename Ts, int i>
typename std::enable_if<(i == 0)>::type appendIdToVec(std::vector<uint16_t>&) {}

template <typename Ts, int i>
typename std::enable_if<(i > 0)>::type appendIdToVec(
    std::vector<uint16_t>& vec) {
  vec.push_back(std::tuple_element<i - 1, Ts>::type::id);
  /* TODO C++17 use constexpr if and remove above function and enable_if
  if constexpr (i > 0) {
      appendIdToVec<Ts, i-1>(vec);
  }
  */
  appendIdToVec<Ts, i - 1>(vec);
}
}  // namespace

//! Manages connection to a Zusi server
class Connection {
 public:
  Connection(Socket* socket) : m_socket(socket) {}

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
  bool connect(const std::string& client_id,
               const std::vector<FuehrerstandData>& fs_data,
               const std::vector<uint16_t>& prog_data, bool bedienung);

  template <typename FtdT, typename ProgDataT>
  bool connect(const std::string& client_id, bool bedienung) {
    std::vector<uint16_t> ftds, progdata;

    appendIdToVec<FtdT, std::tuple_size<FtdT>::value>(ftds);
    appendIdToVec<ProgDataT, std::tuple_size<ProgDataT>::value>(progdata);

    return connect(client_id, ftds, progdata, bedienung);
  }

  //! Receive a message
  std::unique_ptr<zusi::BaseMessage> receiveMessage() const;

  //! Send a message
  bool sendMessage(const Node& src);

  //! Check if there is data read
  bool dataAvailable() { return m_socket->DataToRead(); }

  Node readNodeWithHeader() const;

  /**
   * @brief Send an INPUT command
   * @return True on success
   */
  bool sendInput(const In::Taster& taster, const In::Kommando& kommando,
                 const In::Aktion& aktion, uint16_t position);

  //! Get the version string supplied by the server
  std::string getZusiVersion() { return m_zusiVersion; }

  //! Get the connection info string supplied by the server
  std::string getConnectionnfo() { return m_connectionInfo; }

 private:
  Node readNode() const;
  Attribute readAttribute(uint32_t length) const;
  void writeAttribute(const Attribute& att) const;

  // TODO: ClientConnecton's calls to this via sendMessage()?
  bool writeNode(const Node& node) const;

  Socket* m_socket;
  std::string m_zusiVersion;
  std::string m_connectionInfo;
};
}
