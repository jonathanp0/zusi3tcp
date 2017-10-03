/*
Copyright (c) 2017 Johannes Schlüter

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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <thread>

namespace {
class PosixSocket : public zusi::Socket {
  int sock;
  const char* ip_address;
  unsigned short port;

 public:
  PosixSocket(const char* ip_address, unsigned short port)
      : sock(0), ip_address(ip_address), port(port) {}

  bool connect() {
    // TODO: IPv6, hostname, ... !?
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
      return false;
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_aton(ip_address, &address.sin_addr);

    return ::connect(sock, (struct sockaddr*)&address, sizeof(address)) == 0;
  }

  // WinsockBlockingSocket(SOCKET socket);

  ~PosixSocket() {
    if (sock != -1) {
      close(sock);
    }
  }

  virtual int ReadBytes(void* dest, int bytes) {
    return recv(sock, static_cast<char*>(dest), bytes, MSG_WAITALL);
  }
  virtual int WriteBytes(const void* src, int bytes) {
    return send(sock, static_cast<const char*>(src), bytes, 0);
  }
  virtual bool DataToRead() {
    unsigned long bytes_available;
    if (ioctl(sock, FIONREAD, &bytes_available) != 0) {
      return false;
    }
    return bytes_available > 0;
  }
};

void parseDataMessage(std::unique_ptr<zusi::BaseMessage>&& msg) {
  auto ftdmsg = dynamic_cast<const zusi::FtdDataMessage*>(msg.get());
  auto opmsg = dynamic_cast<const zusi::OperationDataMessage*>(msg.get());
  auto progmsg = dynamic_cast<const zusi::ProgDataMessage*>(msg.get());
  if (ftdmsg) {
    std::cout << "FTD message\n";
    auto speed = ftdmsg->get<zusi::FS::Geschwindigkeit>();
    if (speed) {
      std::cout << "   " << *speed * 3.6f << "km/h\n";
    }

    auto gesamtweg = ftdmsg->get<zusi::FS::Gesamtweg>();
    if (gesamtweg) {
      std::cout << "   Gesamtweg: " << *gesamtweg << std::endl;
    }

    auto sifa = ftdmsg->get<zusi::FS::Sifa>();
    if (sifa) {
      std::cout << "   Sifa Bauart: " << *sifa->getAtt<zusi::Sifa::Bauart>()
                << "\n"
                << "   Sifa Leuchtmelder: "
                << (*sifa->getAtt<zusi::Sifa::Leuchtmelder>() ? "AN" : "aus")
                << "\n"
                << "   Sifa Hupe: "
                << (*sifa->getAtt<zusi::Sifa::Hupe>() ? "AN" : "aus") << "\n";
    }

  } else if (opmsg) {
    std::cout << "OpMsg, " << opmsg->getNodes().size() << " nodes\n";
    for (const zusi::Node& input : opmsg->getNodes()) {
      if (input.getId() == 1) {
        std::cout << "   Tastur Operation:" << std::endl;
        for (const zusi::Attribute& att : input.attributes) {
          if (att.getId() <= 0x3)
            std::cout << "   Parameter " << att.getId() << " = "
                      << att.getValue<uint16_t>() << std::endl;
          else if (att.getId() == 0x4)
            std::cout << "   Position = " << att.getValue<uint16_t>()
                      << std::endl;
        }
      } else {
        std::cout << "    Op Input Id: " << input.getId() << "\n";
      }
    }
  } else if (progmsg) {
    std::cout << "Progmsg\n";
  }
}
}

int main() {
  using namespace std::chrono_literals;
  PosixSocket sock{"192.168.2.25", 1436};
  do {
    if (sock.connect()) {
      try {
        std::cout << "Connected!\n";

        zusi::ClientConnection con(&sock);
        std::vector<zusi::FuehrerstandData> fd_ids{
            zusi::FS::Geschwindigkeit::id, zusi::FS::Gesamtweg::id,
            zusi::FS::Sifa::id};
        std::vector<zusi::ProgData> prog_ids{zusi::ProgData::SimStart};

        // Subscribe to receive status updates about the above variables
        // Not subscribing to input events
        con.connect("Zusi3TCPCli", fd_ids, prog_ids, true);

        std::cout << "Zusi Version:" << con.getZusiVersion() << std::endl;
        std::cout << "Connection Info: " << con.getConnectionnfo() << std::endl;

        do {
          auto msg{con.receiveMessage()};
          // msg.write(debug_socket); //Print data to the console

          parseDataMessage(std::move(msg));
        } while (true);

        //}
      } catch (std::domain_error err) {
        std::cerr << "Domain Exception: " << err.what() << std::endl;
      } catch (std::runtime_error err) {
        std::cerr << "Runtime Exception: " << err.what() << std::endl;
      }
    }
    std::this_thread::sleep_for(10s);
  } while (true);
}
