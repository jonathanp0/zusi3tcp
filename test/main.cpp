#include "Zusi3TCP.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace {
using namespace zusi;

TEST(AttribTag, constexpr) {
  constexpr FS::Gesamtweg distance{1.23f};
  constexpr float dist = distance;
  EXPECT_EQ(dist, 1.23f);
}

TEST(AttribTag, implicitConversion) {
  FS::Geschwindigkeit speed{1.2f};
  Attribute att = speed;
  FS::Geschwindigkeit speed2{att};
  float speedf = speed2;
  EXPECT_EQ(speedf, 1.2f);
}

TEST(AttribTag, comparison) {
  EXPECT_EQ(FS::AfbEinAus{1}, FS::AfbEinAus{1});
  EXPECT_NE(FS::AfbEinAus{1}, FS::AfbEinAus{2});
}

TEST(AttribTag, invalidAttributeConvesion) {
  EXPECT_ANY_THROW({
    Attribute a{FS::LuftpresserLaeuft::id};
    a.setValue<float>(123);
    FS::Streckenhoechstgeschwindigkeit{a};
  });
}

TEST(AttribTag, writeableByRef) {
  FS::UhrzeitMinute min{30};
  EXPECT_EQ(static_cast<int>(min), 30);
  *min = 45;
  EXPECT_EQ(static_cast<int>(min), 45);
}

TEST(AttribTag, size) {
  using FloatAttrib = AttribTag<1, float>;
  EXPECT_EQ(sizeof(FloatAttrib), sizeof(float));
  EXPECT_EQ(sizeof(AttribTag<1, float, bool>), sizeof(bool));
  EXPECT_EQ(sizeof(AttribTag<1, int64_t>), sizeof(int64_t));
}

TEST(AttribTag, string) {
  static const std::string v{"1.2.3.4"};

  HelloAck::ZusiVersion version_in{v};
  EXPECT_TRUE(*version_in == v);

  Attribute attr = version_in;

  EXPECT_EQ(attr.getValueLen(), v.size());

  HelloAck::ZusiVersion version_out{attr};

  // EXPECT_STREQ(version_out, v);
  EXPECT_TRUE(version_out == v);
}

TEST(Node, copyableSingle) {
  const Node n1{1};
  const Node n2{n1};
  const Node n3 = n1;

  EXPECT_EQ(n2.getId(), n1.getId());
  EXPECT_EQ(n3.getId(), n1.getId());
}

TEST(Node, copyableNested) {
  const Node root{[]() {
    Node root{1};
    Node middle{2};
    Node leaf{3};

    middle.nodes.push_back(leaf);
    root.nodes.push_back(middle);
    return root;
  }()};

  const Node copyRoot = root;
  EXPECT_EQ(copyRoot.getId(), 1);
  EXPECT_EQ(copyRoot.nodes.size(), 1);

  const Node copyMiddle = copyRoot.nodes[0];
  EXPECT_EQ(copyMiddle.getId(), 2);
  EXPECT_EQ(copyMiddle.nodes.size(), 1);

  const Node copyLeaf = copyMiddle.nodes[0];
  EXPECT_EQ(copyLeaf.getId(), 3);
  EXPECT_EQ(copyLeaf.nodes.size(), 0);
}

TEST(Node, movableNested) {
  Node root{1};
  Node middle{2};
  Node leaf{3};

  middle.nodes.push_back(leaf);
  root.nodes.push_back(middle);

  auto &&movedRoot = [](Node &&movedRoot) {
    EXPECT_EQ(movedRoot.getId(), 1);
    EXPECT_EQ(movedRoot.nodes.size(), 1);
    return std::move(movedRoot);
  }(std::move(root));

  auto &&movedMiddle = [](Node &&movedMiddle) {
    EXPECT_EQ(movedMiddle.getId(), 2);
    EXPECT_EQ(movedMiddle.nodes.size(), 1);
    return std::move(movedMiddle);
  }(std::move(movedRoot.nodes[0]));

  [](Node &&movedLeaf) {
    EXPECT_EQ(movedLeaf.getId(), 3);
    EXPECT_EQ(movedLeaf.nodes.size(), 0);
  }(std::move(movedMiddle.nodes[0]));
}

TEST(Node, get) {
  Node n;
  n.attributes.emplace_back(FS::Geschwindigkeit{1.2f}.att());
  n.attributes.emplace_back(FS::Gesamtweg{3.4f}.att());

  Node n2{500};
  using C = ComplexNode<500, FS::DruckBremszylinder>;
  n2.attributes.emplace_back(FS::DruckBremszylinder{5.6f}.att());
  n.nodes.push_back(n2);

  auto speed = n.get<FS::Geschwindigkeit>();
  auto oberstrom = n.get<FS::Oberstrom>();
  const auto gesamtweg = n.get<FS::Gesamtweg>();
  auto complex = n.get<C>();

  EXPECT_EQ(**speed, 1.2f);
  EXPECT_FALSE(oberstrom);
  EXPECT_EQ(**gesamtweg, 3.4f);
  EXPECT_EQ(*complex->get<FS::DruckBremszylinder>(), 5.6f);
}

TEST(ComplexNode, simple) {
  using ComplexTestNode =
      ComplexNode<23, FS::Geschwindigkeit, FS::DruckHauptlufleitung,
                  FS::UhrzeitStunde>;

  ComplexTestNode testNode;

  FS::UhrzeitStunde &att1 = testNode.get<FS::UhrzeitStunde>();
  *att1 = 23;
  FS::UhrzeitStunde att2 = testNode.get<FS::UhrzeitStunde>();
  EXPECT_EQ(23, *att2);
}

TEST(ComplexNode, decode) {
  using ComplexTestNode =
      ComplexNode<23, FS::Geschwindigkeit, FS::DruckHauptlufleitung,
                  FS::UhrzeitStunde>;

  Node node{23};
  node.attributes.emplace_back((FS::Geschwindigkeit{10}).att());
  node.attributes.emplace_back((FS::UhrzeitStunde{20}).att());
  node.attributes.emplace_back((FS::DruckHauptlufleitung{30}).att());

  ComplexTestNode complex{node};
  EXPECT_EQ(10, complex.get<FS::Geschwindigkeit>());
  EXPECT_EQ(20, complex.get<FS::UhrzeitStunde>());
  EXPECT_EQ(30, complex.get<FS::DruckHauptlufleitung>());
}

TEST(ComplexNode, missingAttribute) {
  using ComplexTestNode =
      ComplexNode<23, FS::Geschwindigkeit, FS::DruckHauptlufleitung,
                  FS::UhrzeitStunde>;

  Node node{23};
  node.attributes.emplace_back((FS::Geschwindigkeit{10}).att());
  node.attributes.emplace_back((FS::UhrzeitStunde{20}).att());
  // Not there:
  // node.attributes.emplace_back((FS::DruckHauptlufleitung{30}).att());

  EXPECT_ANY_THROW({ ComplexTestNode{node}; });
}

  /*
   Using Mock stuff would be nice, but I am too stupid to return by pointer
  class MockSocket : public Socket {
  public:
      MOCK_METHOD2(ReadBytes, int(void*, int));
      MOCK_METHOD2(WriteBytes, int(const void*, int));
      MOCK_METHOD0(DataToRead, bool());
  };

  TEST(Connection, readXXXXXXXXXXXXX) {
      using ::testing::_;
      using ::testing::Return;
      using ::testing::SetArgPointee;
      using ::testing::DoAll;

      MockSocket sock{};
      Connection conn{&sock};

      uint8_t header{0};
      EXPECT_CALL(sock, ReadBytes(_, _)).
              WillOnce(
                  //DoAll(
                      //SetArgPointee<0>((void*)&header),
                      Return(sizeof(uint8_t))
                  //)
              );
      conn.receiveMessage();
  }
  */

#ifdef FAKE_SOCKET
class FakeSocket : public Socket {
  const char *readdata;
  size_t len;
  char *readpos;

  std::vector<uint8_t> writedata;

 public:
  FakeSocket() : readdata(""), len(0), readpos(const_cast<char *>(readdata)) {}
  FakeSocket(const char *data, size_t len)
      : readdata(data), len(len), readpos(const_cast<char *>(data)) {}

  bool connect() override { return true; }

  int ReadBytes(void *dest, int bytes) override {
#ifdef DUMP_FAKE_STREAM_READS
    std::cout << "At " << readpos - readdata << " reading " << bytes << " of "
              << len << " remaining, starting at " << &readpos << std::endl;
#endif
    EXPECT_LE(bytes, len);
    memcpy(dest, readpos, bytes);
    readpos += bytes;
    len -= bytes;
    return bytes;
  }

  bool DataToRead() override { return len != 0; }

  int WriteBytes(const void *src, int len) override {
    const uint8_t *data{reinterpret_cast<const uint8_t *>(src)};
    std::copy(data, data + len, std::back_inserter(writedata));
    return len;
  }

  const std::vector<uint8_t> &GetWrittenData() { return writedata; }
};

TEST(Connection, readSingleNode) {
  static const char data[] =
      "\x00\x00\x00\x00"  // header"
      "\x01\x00"          // first node's id
      "\xff\xff\xff\xff"  // end of node
      ;

  FakeSocket sock{data, sizeof(data) - 1};
  Connection conn{&sock};

  Node n{conn.readNodeWithHeader()};
  EXPECT_EQ(n.getId(), 1);
  EXPECT_EQ(n.attributes.size(), 0);
  EXPECT_EQ(n.nodes.size(), 0);
}

TEST(Connection, readNestedNode) {
  static const char data[] =
      "\x00\x00\x00\x00"  // header"
      "\x01\x00"          // first node's id

      "\x00\x00\x00\x00"  // begin child node
      "\x02\x00"          // child node's id
      "\xff\xff\xff\xff"  // end of child node

      "\xff\xff\xff\xff"  // end of root node
      ;

  FakeSocket sock{data, sizeof(data) - 1};
  Connection conn{&sock};

  Node n{conn.readNodeWithHeader()};
  EXPECT_FALSE(sock.DataToRead());
  EXPECT_EQ(n.getId(), 1);
  EXPECT_EQ(n.attributes.size(), 0);
  EXPECT_EQ(n.nodes.size(), 1);
}

TEST(Connection, readNestedAttribute) {
  // Example data from manual
  static const char data[] =
      "\x00\x00\x00\x00"  // header"
      "\x02\x00"          // first node's id

      "\x00\x00\x00\x00"  // child node begin
      "\x0a\x00"          // FTD Data

      "\x06\x00\x00\x00"  // Attribute of size 6
      "\x01\x00"          // ID = 1 -> Speed
      "\xae\x47\x3d\x41"  // 11.83 m/s

      "\x06\x00\x00\x00"  // Attribute of size 6
      "\x1b\x00"          // ID = 0x1B -> LM Scleudern
      "\x00\x00\x00\x00"  // 0 / Off

      "\xff\xff\xff\xff"  // end of child node
      "\xff\xff\xff\xff"  // end of root node
      ;

  FakeSocket sock{data, sizeof(data) - 1};
  Connection conn{&sock};

  const auto msg{conn.receiveMessage()};
  EXPECT_FALSE(sock.DataToRead());

  auto ftdmsg = dynamic_cast<const FtdDataMessage *>(msg.get());
  EXPECT_TRUE(ftdmsg);

  // Check attribs
  bool has_speed = ftdmsg->has<FS::Geschwindigkeit>();
  EXPECT_TRUE(has_speed);
  auto speed = ftdmsg->get<FS::Geschwindigkeit>();
  EXPECT_TRUE(speed);
  EXPECT_FLOAT_EQ(*speed, 11.83f);

  bool has_schleudern = ftdmsg->has<FS::LMSchleudern>();
  EXPECT_TRUE(has_schleudern);
  auto schleudern = ftdmsg->get<FS::LMSchleudern>();
  EXPECT_TRUE(schleudern);
  EXPECT_FALSE(*schleudern);
}

static std::string ToHex(const std::string &s, bool upper_case = true) {
  std::ostringstream ret;

  for (std::string::size_type i = 0; i < s.length(); ++i)
    ret << std::hex << std::setfill('0') << std::setw(2)
        << (upper_case ? std::uppercase : std::nouppercase) << (int)s[i];

  return ret.str();
}

// This test in effect tests two different things:
// 1) It sends a HELLO message and verifies it's encoded correctly
// 2) It receives a HELLO_ACK and checks those are parsed correctly
// Those two things should be split, or maybe use ServerConnection
TEST(ClientConnection, connect) {
  // Example data from manual
  static const char requestdata[] =
      // Welcome message
      "\x00\x00\x00\x00"  // root node
      "\x01\x00"          // connection request
      "\x00\x00\x00\x00"  // child node
      "\x01\x00"          // 1 -> Hello

      "\x04\x00\x00\x00"  // Attribute 4 byte long
      "\x01\x00"          // Protocol version
      "\x02\x00"          // Version 2

      "\x04\x00\x00\x00"  // Attribute 4 byte long
      "\x02\x00"          // Client type
      "\x02\x00"          // Controller

      "\x0c\x00\x00\x00"  // Attribute 12 bytes
      "\x03\x00"          // name
      "testclient"        // literal name

      "\x05\x00\x00\x00"  // Attribute 5 byte long
      "\x04\x00"          // version
      "2.0"               // literal name

      "\xff\xff\xff\xff"  // end of child node
      "\xff\xff\xff\xff"  // end of root node

      "\x00\x00\x00\x00"  // root node
      "\x02\x00"
      "\x00\x00\x00\x00"  // child node
      "\x03\x00"

      "\x00\x00\x00\x00"  // child node
      "\x0A\x00"

      "\x04\x00\x00\x00"  // Attribute length 4
      "\x01\x00"          // type=FT ID
      "\x01\x00"          // Speed
      "\x04\x00\x00\x00"  // Attribute length 4
      "\x01\x00"          // type=FT ID
      "\x1B\x00"          // Schleudern

      "\xff\xff\xff\xff"  // end of child node

      "\xff\xff\xff\xff"  // end of child node
      "\xff\xff\xff\xff"  // end of root node
      ;

  static const char serverdata[] =
      // ACK Hello
      "\x00\x00\x00\x00"  // header
      "\x01\x00"          // connect

      "\x00\x00\x00\x00"  // child node
      "\x02\x00"          // ACK HELLO

      "\x09\x00\x00\x00"  // Attribute 9 bytes
      "\x01\x00"          // Version
      "3.0.1.0"           // literal

      "\x03\x00\x00\x00"  // Attribute 3 bytes long
      "\x02\x00"          // connection info
      "0"                 // literal

      "\x03\x00\x00\x00"  // Attribute 3 bytes long
      "\x03\x00"          // Result
      "\x00"              // OK

      "\xff\xff\xff\xff"  // end of child node
      "\xff\xff\xff\xff"  // end of root node

      // Request data
      "\x00\x00\x00\x00"  // root node
      "\x02\x00"          // fahrpulte

      "\x00\x00\x00\x00"  // child node
      "\x04\x00"          // ACK_NEEDED_DATA

      "\x03\x00\x00\x00"  // Attribute 3 byte long
      "\x01\x00"          // result
      "\x00"              // result, 0 -> ok

      "\xff\xff\xff\xff"  // end of child node
      "\xff\xff\xff\xff"  // end of root node
      ;

  FakeSocket sock{serverdata, sizeof(serverdata) - 1};
  Connection conn{&sock};

  EXPECT_NO_THROW({
    conn.connect("testclient", {FS::Geschwindigkeit::id, FS::LMSchleudern::id},
                 {}, false);
  });

  auto written = sock.GetWrittenData();
  std::string writtens{reinterpret_cast<const char *>(written.data()),
                       written.size()};
  std::string expected{requestdata, sizeof(requestdata) - 1};
  // EXPECT_STREQ(writtens, expected);

  /*
  std::cout << writtens.size() << "-" << expected.size() << "\n";
  std::cout << ToHex(writtens) << "\n";
  std::cout << ToHex(expected) << "\n";
  */

  EXPECT_TRUE(writtens == expected);

  // EXPECT_STREQ(conn.getZusiVersion(), "3.0.1.0");
  EXPECT_TRUE(conn.getZusiVersion() == "3.0.1.0");
}

// This is almost the same as above, but uses type-based registration and
// reorderd Ftd-List
TEST(ClientConnection, connectTyped) {
  // Example data from manual
  static const char requestdata[] =
      // Welcome message
      "\x00\x00\x00\x00"  // root node
      "\x01\x00"          // connection request
      "\x00\x00\x00\x00"  // child node
      "\x01\x00"          // 1 -> Hello

      "\x04\x00\x00\x00"  // Attribute 4 byte long
      "\x01\x00"          // Protocol version
      "\x02\x00"          // Version 2

      "\x04\x00\x00\x00"  // Attribute 4 byte long
      "\x02\x00"          // Client type
      "\x02\x00"          // Controller

      "\x0c\x00\x00\x00"  // Attribute 12 bytes
      "\x03\x00"          // name
      "testclient"        // literal name

      "\x05\x00\x00\x00"  // Attribute 5 byte long
      "\x04\x00"          // version
      "2.0"               // literal name

      "\xff\xff\xff\xff"  // end of child node
      "\xff\xff\xff\xff"  // end of root node

      "\x00\x00\x00\x00"  // root node
      "\x02\x00"
      "\x00\x00\x00\x00"  // child node
      "\x03\x00"

      "\x00\x00\x00\x00"  // child node
      "\x0A\x00"

      "\x04\x00\x00\x00"  // Attribute length 4
      "\x01\x00"          // type=FT ID
      "\x01\x00"          // Speed
      "\x04\x00\x00\x00"  // Attribute length 4
      "\x01\x00"          // type=FT ID
      "\x1B\x00"          // Schleudern

      "\xff\xff\xff\xff"  // end of child node

      "\xff\xff\xff\xff"  // end of child node
      "\xff\xff\xff\xff"  // end of root node
      ;

  static const char serverdata[] =
      // ACK Hello
      "\x00\x00\x00\x00"  // header
      "\x01\x00"          // connect

      "\x00\x00\x00\x00"  // child node
      "\x02\x00"          // ACK HELLO

      "\x09\x00\x00\x00"  // Attribute 9 bytes
      "\x01\x00"          // Version
      "3.0.1.0"           // literal

      "\x03\x00\x00\x00"  // Attribute 3 bytes long
      "\x02\x00"          // connection info
      "0"                 // literal

      "\x03\x00\x00\x00"  // Attribute 3 bytes long
      "\x03\x00"          // Result
      "\x00"              // OK

      "\xff\xff\xff\xff"  // end of child node
      "\xff\xff\xff\xff"  // end of root node

      // Request data
      "\x00\x00\x00\x00"  // root node
      "\x02\x00"          // fahrpulte

      "\x00\x00\x00\x00"  // child node
      "\x04\x00"          // ACK_NEEDED_DATA

      "\x03\x00\x00\x00"  // Attribute 3 byte long
      "\x01\x00"          // result
      "\x00"              // result, 0 -> ok

      "\xff\xff\xff\xff"  // end of child node
      "\xff\xff\xff\xff"  // end of root node
      ;

  FakeSocket sock{serverdata, sizeof(serverdata) - 1};
  Connection conn{&sock};

  conn.connect<std::tuple<FS::LMSchleudern, FS::Geschwindigkeit>, std::tuple<>>(
      "testclient", false);

  auto written = sock.GetWrittenData();
  std::string writtens{reinterpret_cast<const char *>(written.data()),
                       written.size()};
  std::string expected{requestdata, sizeof(requestdata) - 1};

  EXPECT_TRUE(writtens == expected);

  // EXPECT_STREQ(conn.getZusiVersion(), "3.0.1.0");
  EXPECT_TRUE(conn.getZusiVersion() == "3.0.1.0");
}

TEST(ClientConnection, sendInput) {
  const char expected[] =
      "\x00\x00\x00\x00"
      "\x02\x00"
      "\x00\x00\x00\x00"
      "\x0A\x01"
      "\x00\x00\x00\x00"
      "\x01\x00"
      "\x04\x00\x00\x00"
      "\x01\x00"
      "\x01\x00"
      "\x04\x00"
      "\x00\x00"
      "\x02\x00"
      "\x00\x00"
      "\x04\x00"
      "\x00\x00"
      "\x03\x00"
      "\x07\x00"
      "\x04\x00\x00\x00\x04\x00\x0A\x00\x06\x00\x00\x00\x05\x00\x00\x00\x00\x00"
      "\xFF\xFF\xFF\xFF"
      "\xFF\xFF\xFF\xFF"
      "\xFF\xFF\xFF\xFF";
  FakeSocket sock{};
  Connection conn{&sock};

  conn.sendInput(Taster::Fahrschalter, Kommando::Unbestimmt, Aktion::Absolut,
                 10);

  std::string expecteds{expected, sizeof(expected) - 1};
  // std::cout << ToHex(expecteds) << '\n';

  auto written = sock.GetWrittenData();
  std::string writtens{reinterpret_cast<const char *>(written.data()),
                       written.size()};
  // std::cout << ToHex(writtens) << '\n';

  // EXPECT_STREQ(writtens, expecteds);
  EXPECT_TRUE(writtens == expecteds);
}

#endif

TEST(OperationDataMessage, iteratorEmpty) {
  Node opnode{0x0b};

  for (auto bedienung : OperationDataMessage{opnode}) {
    EXPECT_FALSE(bedienung.id);
  }
}
TEST(OperationDataMessage, iteratorNotEmpty) {
  Node input1{0x01};
  input1.attributes.push_back(Taster::AFB);
  input1.attributes.push_back(Kommando::FahrschalterAb_Up);
  input1.attributes.push_back(Aktion::AufUp);
  input1.attributes.push_back(In::Position{0});
  input1.attributes.push_back(In::Spezial{0});

  Node input2{0x01};
  input2.attributes.push_back(Taster::Hauptschalter);
  input2.attributes.push_back(Kommando::PfeifeDown);
  input2.attributes.push_back(Aktion::Down);
  input2.attributes.push_back(In::Position{1});
  input2.attributes.push_back(In::Spezial{1});

  Node opnode{0x0b};
  opnode.nodes.push_back(input1);
  opnode.nodes.push_back(input2);

  const auto opmsg = OperationDataMessage{opnode};
  int count_b{0};
  for (auto it = opmsg.begin(); it != opmsg.end(); ++it) {
    ASSERT_LE(++count_b, 2);
  }
  EXPECT_EQ(count_b, 2);

  int count{0};
  for (auto bedienung : OperationDataMessage{opnode}) {
    ASSERT_LE(++count, 2);
    if (count == 1) {
      EXPECT_EQ(bedienung.get<In::Taster>(), Taster::AFB);
      EXPECT_EQ(bedienung.get<In::Kommando>(), Kommando::FahrschalterAb_Up);
      EXPECT_EQ(bedienung.get<In::Aktion>(), Aktion::AufUp);
      EXPECT_EQ(*bedienung.get<In::Position>(), 0);
      EXPECT_EQ(*bedienung.get<In::Spezial>(), 0);
    } else if (count == 2) {
      EXPECT_EQ(bedienung.get<In::Taster>(), Taster::Hauptschalter);
      EXPECT_EQ(bedienung.get<In::Kommando>(), Kommando::PfeifeDown);
      EXPECT_EQ(bedienung.get<In::Aktion>(), Aktion::Down);
      EXPECT_EQ(*bedienung.get<In::Position>(), 1);
      EXPECT_EQ(*bedienung.get<In::Spezial>(), 1);
    }
  }
  EXPECT_EQ(count, 2);
}

TEST(OperationDataMessage, TODOignoreKombiHebelForNow) {  // TODO!
  Node input0{0x02};

  Node input1{0x01};
  input1.attributes.push_back(Taster::AFB);
  input1.attributes.push_back(Kommando::FahrschalterAb_Up);
  input1.attributes.push_back(Aktion::AufUp);
  input1.attributes.push_back(In::Position{0});
  input1.attributes.push_back(In::Spezial{0});

  Node input2{0x02};

  Node input3{0x01};
  input3.attributes.push_back(Taster::Hauptschalter);
  input3.attributes.push_back(Kommando::PfeifeDown);
  input3.attributes.push_back(Aktion::Down);
  input3.attributes.push_back(In::Position{1});
  input3.attributes.push_back(In::Spezial{1});

  Node input4{0x02};

  Node opnode{0x0b};
  opnode.nodes.push_back(input0);
  opnode.nodes.push_back(input1);
  opnode.nodes.push_back(input2);
  opnode.nodes.push_back(input3);
  opnode.nodes.push_back(input4);

  int count{0};
  for (auto bedienung : OperationDataMessage{opnode}) {
    ASSERT_LE(++count, 2);
    if (count == 1) {
      EXPECT_EQ(bedienung.get<In::Taster>(), Taster::AFB);
      EXPECT_EQ(bedienung.get<In::Kommando>(), Kommando::FahrschalterAb_Up);
      EXPECT_EQ(bedienung.get<In::Aktion>(), Aktion::AufUp);
      EXPECT_EQ(*bedienung.get<In::Position>(), 0);
      EXPECT_EQ(*bedienung.get<In::Spezial>(), 0);
    } else if (count == 2) {
      EXPECT_EQ(bedienung.get<In::Taster>(), Taster::Hauptschalter);
      EXPECT_EQ(bedienung.get<In::Kommando>(), Kommando::PfeifeDown);
      EXPECT_EQ(bedienung.get<In::Aktion>(), Aktion::Down);
      EXPECT_EQ(*bedienung.get<In::Position>(), 1);
      EXPECT_EQ(*bedienung.get<In::Spezial>(), 1);
    }
  }
  EXPECT_EQ(count, 2);
}

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleMock(&argc, argv);
  int ret = RUN_ALL_TESTS();
  return ret;
}
