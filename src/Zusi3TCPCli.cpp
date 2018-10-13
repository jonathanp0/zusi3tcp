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

using namespace std::literals;
using boost::asio::ip::tcp;
using namespace zusi;

namespace {
class dumpVisitor : boost::static_visitor<void> {
 public:
  void operator()(const FtdDataMessage& ftdmsg) const {
    std::cout << "FTD message\n";
    auto speed = ftdmsg.get<FS::Geschwindigkeit>();
    if (speed) {
      std::cout << "   " << *speed * 3.6f << "km/h\n";
    }

    auto gesamtweg = ftdmsg.get<FS::Gesamtweg>();
    if (gesamtweg) {
      std::cout << "   Gesamtweg: " << *gesamtweg << std::endl;
    }

    auto sifa = ftdmsg.get<FS::Sifa>();
    if (sifa) {
      std::cout << "   Sifa Bauart: " << *sifa->get<Sifa::Bauart>() << "\n"
                << "   Sifa Leuchtmelder: "
                << (*sifa->get<Sifa::Leuchtmelder>() ? "AN" : "aus") << "\n"
                << "   Sifa Hupe: " << (*sifa->get<Sifa::Hupe>() ? "AN" : "aus")
                << "\n";
    }
  }

  void operator()(const OperationDataMessage& opmsg) const {
    for (auto op : opmsg) {
      std::cout << "Operation:\n";
      std::cout << "    Kommando " << *op.get<In::Kommando>() << "\n";
    }
  }

  void operator()(const ProgDataMessage& progmsg) const {
    std::cout << "Progmsg\n";
    auto zugnummer = progmsg.get<ProgData::Zugnummer>();
    if (zugnummer) {
      std::cout << "Zugnummer: " << **zugnummer << '\n';
    }
    auto zugdatei = progmsg.get<ProgData::BuchfahrplanDatei>();
    if (zugdatei) {
      std::cout << "Zugdatei: " << **zugdatei << '\n';
    }
  }
};

void dumpData(const MessageVariant& msg) {
  boost::apply_visitor(dumpVisitor{}, msg);
}
}  // namespace

int main() {
  boost::asio::io_service io_service;
  tcp::socket socket{io_service};

  std::cout << "Running in endless loop, trying to connect. Use Ctrl-C to "
               "terminate.\n";
  do {
    std::cout << "Trying ... \n";

    boost::system::error_code error;
    tcp::resolver resolver(io_service);
    tcp::resolver::query query{"192.168.2.22"s, "1436"s};
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    boost::asio::connect(socket, endpoint_iterator, error);

    if (!error) {
      try {
        Connection con(socket);
        con.connect<std::tuple<FS::Geschwindigkeit, FS::Gesamtweg, FS::Sifa>,
                    std::tuple<ProgData::SimStart, ProgData::Zugnummer,
                               ProgData::BuchfahrplanDatei> >("Zusi3TCPCli",
                                                              true);

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
        std::cerr << "An unexpected exception type, will terminate."
                  << std::endl;
        throw;
      }
    }
    std::this_thread::sleep_for(10s);
  } while (true);
}
