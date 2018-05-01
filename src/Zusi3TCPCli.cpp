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

#include <iostream>
#include <thread>

#define BOOST_ERROR_CODE_HEADER_ONLY 1
#include <boost/array.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;
namespace {
class BoostAsioSyncSocket : public zusi::Socket {
  boost::asio::io_service io_service;
  tcp::resolver::query query;
  tcp::socket socket;

 public:
  BoostAsioSyncSocket(std::string&& target, const std::string&& port) : query{target, port}, socket{io_service} {
  }

  bool connect() override {
    boost::system::error_code error;
    tcp::resolver resolver(io_service);
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    boost::asio::connect(socket, endpoint_iterator, error);
    return !error;
  }

  virtual int ReadBytes(void* dest, int bytes) override {
    boost::system::error_code error;
    int len = 0;
    do {
      /* TODO: Handle error */
      len += socket.read_some(
          boost::asio::buffer((char*)dest + len, bytes - len), error);
    } while (bytes > len);
    return len;
  }
  virtual int WriteBytes(const void* src, int bytes) override {
      return boost::asio::write(socket, boost::asio::buffer(src, bytes)/*, ignored_error*/);
  }
  virtual bool DataToRead() override { return socket.available() > 0; }
};


void dumpData(std::unique_ptr<zusi::BaseMessage>&& msg) {
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
      std::cout << "   Sifa Bauart: " << *sifa->get<zusi::Sifa::Bauart>()
                << "\n"
                << "   Sifa Leuchtmelder: "
                << (*sifa->get<zusi::Sifa::Leuchtmelder>() ? "AN" : "aus")
                << "\n"
                << "   Sifa Hupe: "
                << (*sifa->get<zusi::Sifa::Hupe>() ? "AN" : "aus") << "\n";
    }

  } else if (opmsg) {
    for (auto op : *opmsg) {
        std::cout << "Operation:\n";
		std::cout << "    Kommando " << *op.get<zusi::In::Kommando>() << "\n";
    }
  } else if (progmsg) {
    std::cout << "Progmsg\n";
    auto zugnummer = progmsg->get<zusi::ProgData::Zugnummer>();
    if (zugnummer) {
        std::cout << "Zugnummer: " << **zugnummer << '\n';
    }
    auto zugdatei = progmsg->get<zusi::ProgData::BuchfahrplanDatei>();
    if (zugdatei) {
    std::cout << "Zugdatei: " << **zugdatei << '\n';
    }

  }
}
}


int main() {
  using namespace std::literals;
  //PosixSocket sock{"192.168.2.22", 1436};
  BoostAsioSyncSocket sock{"192.168.2.22"s, "1436"s};
  std::cout << "Running in endless loop, trying to connect. Use Ctrl-C to terminate.\n";
  do {
    std::cout << "Trying ... \n";
    if (sock.connect()) {
      try {
        zusi::ClientConnection con(&sock);
        con.connect<std::tuple<zusi::FS::Geschwindigkeit, zusi::FS::Gesamtweg,
                               zusi::FS::Sifa>,
                    std::tuple<zusi::ProgData::SimStart, zusi::ProgData::Zugnummer, zusi::ProgData::BuchfahrplanDatei> >("Zusi3TCPCli", true);

        std::cout << "Zusi Version:" << con.getZusiVersion() << std::endl;
        std::cout << "Connection Info: " << con.getConnectionnfo() << std::endl;

        do {
          auto msg{con.receiveMessage()};

          dumpData(std::move(msg));
        } while (true);

      } catch (std::domain_error& err) {
        std::cerr << "Domain Exception: " << err.what() << std::endl;
      } catch (std::runtime_error& err) {
        std::cerr << "Runtime Exception: " << err.what() << std::endl;
      } catch (...) {
        std::cerr << "An unexpected exception type, will terminate." << std::endl;
        throw;
      }
    }
    std::this_thread::sleep_for(10s);
  } while (true);
}
