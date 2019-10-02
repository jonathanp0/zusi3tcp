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

#include <chrono>
#include <set>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#define BOOST_SYSTEM_NO_LIB
#include <boost/asio.hpp>
#include <boost/variant.hpp>

#include "Zusi3TCPData.h"

//! Zusi Namespace
namespace zusi {

class FtdDataMessage {
  const Node root;

 public:
  FtdDataMessage(const Node root) : root{std::move(root)} {}
  FtdDataMessage(Node&& root) : root{root} {}

  FtdDataMessage(const FtdDataMessage&) = default;
  FtdDataMessage(FtdDataMessage&&) = default;

  template <typename T>
  std::optional<T> get() const noexcept {
    return root.get<T>();
  }

  template <typename T>
  bool has() const noexcept {
    return root.get<T>();
  }
};

class OperationDataMessage {
  const Node root;

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

  OperationDataMessage(const Node root) : root{std::move(root)} {}
  OperationDataMessage(Node&& root) : root{root} {}

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

class ProgDataMessage {
  const Node root;

 public:
  ProgDataMessage(const Node root) : root{std::move(root)} {}
  ProgDataMessage(Node&& root) : root{root} {}

  ProgDataMessage(const ProgDataMessage&) = default;
  ProgDataMessage(ProgDataMessage&&) = default;

  template <typename T>
  std::optional<T> get() const noexcept {
    return root.get<T>();
  }
};

using MessageVariant =
    boost::variant<FtdDataMessage, OperationDataMessage, ProgDataMessage>;

#ifdef OLD_COMPILER_PRE_CPP17
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
#endif

namespace {
template <typename Ts, int i>
void appendIdToVec(std::vector<uint16_t>& vec) {
  if constexpr (i > 0) {
    vec.push_back(std::tuple_element<i - 1, Ts>::type::id);
    appendIdToVec<Ts, i - 1>(vec);
  }
}
}  // namespace

//! Manages connection to a Zusi server
class Connection {
 public:
  Connection(boost::asio::ip::tcp::tcp::socket& socket,
             const std::string& client_id);

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
   * @return True on successnknown CMake command
   */
  bool connect(const std::vector<FuehrerstandData>& fs_data,
               const std::vector<uint16_t>& prog_data, bool bedienung);

  template <typename FtdT, typename ProgDataT>
  bool connect(bool bedienung) {
    std::vector<uint16_t> ftds, progdata;

    appendIdToVec<FtdT, std::tuple_size<FtdT>::value>(ftds);
    appendIdToVec<ProgDataT, std::tuple_size<ProgDataT>::value>(progdata);

    return connect(ftds, progdata, bedienung);
  }

  //! Receive a message
  MessageVariant receiveMessage() const;

  //! Check if there is data read
  bool dataAvailable() const { return m_socket.available() > 0; }

  Node readNodeWithHeader() const;

  /**
   * @brief Send an INPUT command
   * @return True on success
   */
  bool sendInput(const In::Taster& taster, const In::Kommando& kommando,
                 const In::Aktion& aktion, uint16_t position);

  //! Get the version string supplied by the server
  const std::string& getZusiVersion() const { return m_zusiVersion; }

  //! Get the connection info string supplied by the server
  const std::string& getConnectionnfo() const { return m_connectionInfo; }

 private:
  Node readNode() const;
  Attribute readAttribute(uint32_t length) const;
  bool writeNode(const Node& node) const;
  void writeAttribute(const Attribute& att) const;

  boost::asio::ip::tcp::tcp::socket& m_socket;
  std::string m_zusiVersion;
  std::string m_connectionInfo;
};
}  // namespace zusi
