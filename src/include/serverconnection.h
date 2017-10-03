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

#define SERVERCONNECTION_H

#include "Zusi3TCP.h"

namespace zusi {
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

#ifdef USE_OLD_FSDATAITEM
  /**
  * @brief Send FuehrerstandData updates to the client
  *
  * Only data that was requested by the client will actually be sent
  */
  bool sendData(std::vector<FsDataItem*> ftd_items);
#endif

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
