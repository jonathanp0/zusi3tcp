#ifndef ZUSI3TCPDATA_H
#define ZUSI3TCPDATA_H

#include "Zusi3TCPTypes.h"

namespace zusi {

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
using ZusiVerbindungsinfo = AttribTag<2, std::string>;
using ZusiOk = AttribTag<3, uint8_t>;
}  // namespace HelloAck

namespace NeedDataAck {
using ZusiOk = AttribTag<1, uint8_t>;
}

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

}  // namespace zusi
#endif  // ZUSI3TCPDATA_H
