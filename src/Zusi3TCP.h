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

#include <cstdint>
#include <vector>
#include <set>
#include <string>
#include <stdexcept>

#include "string.h" // For memcpy

//! Zusi Namespace
namespace zusi
{
	//! Message Type Node ID - used for root node of message
	enum MsgType
	{
		MsgType_Connecting = 1,
		MsgType_Fahrpult = 2
	};

	//! Command Node ID - type of sub-nodes of root node
	enum Command
	{
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

	//! Program status Data variable ID's
	enum ProgData
	{
		Prog_Zugdatei = 1,
		Prog_Zugnummer,
		Prog_SimStart,
		Prog_BuchfahrplanDatei
	};

	//! Input function ID's
	enum Tastatur
	{
		Tt_Fahrschalter = 1,
		Tt_DynBremse = 2,
		Tt_Pfeife = 0x0C,
		Tt_Sifa = 0x10,
	};

	//! Input command ID's
	enum TastaturKommand
	{
		Tk_Unbestimmt = 0,
		Tk_FahrschalterAuf_Down = 1,
		Tk_FahrschalterAuf_Up = 2,
		Tk_FahrschalterAb_Down = 3,
		Tk_FahrschalterAb_Up = 4,
		Tk_SifaDown = 0x39,
		Tk_SifaUp = 0x3A,
		Tk_PfeifeDown = 0x45,
		Tk_PfeifeUp = 0x46
	};

	//! Input action ID's
	enum TastaturAktion
	{
		Ta_Default = 0,
		Ta_Down = 1,
		Ta_Up = 2,
		Ta_AufDown = 3,
		Ta_AufUp = 4,
		Ta_AbDown = 5,
		Ta_AbUp = 6,
		Ta_Absolut = 7,
		Ta_Absolut1000er = 8
	};

	//! Abstract interface for a socket
	class Socket
	{
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
	class Attribute
	{
	public:

		//! Construct an empty attribute
        Attribute() : data_bytes(0), m_id(0)
		{
		}

		/** @brief Constructs an attribute
		* @param id Attribute ID
		*/
		Attribute(uint16_t id) : data_bytes(0), m_id(id)
		{
		}

        template <typename T>
        Attribute(int16_t id, T value) : data_bytes(0), m_id(id)
        {
            setValue<T>(value);
        }

		Attribute(const Attribute&) = default;
        Attribute(Attribute&&) = default;
		Attribute& operator=(const Attribute& o) = default;

		virtual ~Attribute()
		{
		}

		void write(Socket& sock) const;

		bool read(Socket& sock, uint32_t length);

		//! Get Attribute ID
		uint16_t getId() const
		{
			return m_id;
		}

		//! Utility function to set the value
		template<typename T>
		void setValue(T value)
		{
            static_assert(sizeof(T) <= sizeof(data), "Trying to write too big data into attribute");
			*reinterpret_cast<T*>(data) = value;
			data_bytes = sizeof(T);
		}

        void setValueRaw(const uint8_t *data, size_t length)
        {
            if (length > sizeof(this->data)) {
                throw std::runtime_error("Trying to write too much data");
            }
            data_bytes = length;
            memcpy(this->data, data, length);
        }

        bool hasValue() const
        {
            return data_bytes != 0;
        }

        template<typename T>
        bool hasValueType() const
        {
            return sizeof(T) == data_bytes;
        }

        size_t getValueLen() const
        {
            return data_bytes;
        }

        template<typename T>
        T getValue() const
        {
            static_assert(sizeof(T) <= sizeof(data), "Trying to read too big data into attribute");
            return *reinterpret_cast<const T*>(data);
        }

        const uint8_t *getValueRaw(size_t *len) const
        {
            *len = data_bytes;
            return data;
        }

    private:
		//! Number of bytes in #data
		uint32_t data_bytes;

		//! Attribute data
		uint8_t data[255]; // more or less random length, this won't be enough for the timetable XML, but most likely this client
                           // won't read such large messages correctly either due to ignoring read()/recv()'s return value and
                           // network fragmentation

		uint16_t m_id;
	};

	//! Generic Zusi message node
	class Node
	{
	public:

		//! Constructs an empty node
		Node() : m_id(0)
		{
		}

		/** @brief Constructs an Node
		* @param id Attribute ID
		*/
		Node(uint16_t id) : m_id(id)
		{
		}

		Node(const Node& o) = default;
        Node(Node &&) = default;
		Node& operator=(const Node& o) = default;

		bool write(Socket& sock) const;
		
		bool read(Socket& sock);

		//! Get Attribute ID
		uint16_t getId() const
		{
			return m_id;
		}

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

	It should be comparable so that the application can check if the data has changed
	before transmitting.
	*/
	class FsDataItem
	{
	public:
		FsDataItem(FuehrerstandData id) : m_id(id) {}

		//!Append this data item to the specified node
		virtual void appendTo(Node& node) const = 0;
		virtual bool operator==(const FsDataItem& a) = 0;

		bool operator!=(const FsDataItem& a) {
			return !(*this == a);
		}

		//! Returns the Fuehrerstand Data type ID of this data item
		FuehrerstandData getId() const { return m_id; }
	private:
		FuehrerstandData m_id;
	};

	//! Generic class for a float DATA_FTD item
	class FloatFsDataItem : public FsDataItem
	{
	public:
		FloatFsDataItem(FuehrerstandData id, float value) : FsDataItem(id), m_value(value) {}

		virtual bool operator==(const FsDataItem& a){
			if (getId() != a.getId())
				return false;

			const FloatFsDataItem* pA = dynamic_cast<const FloatFsDataItem*>(&a);
			if(pA && pA->m_value == m_value)
				return true;
			
			return false;
		}

		virtual void appendTo(Node& node) const
		{ 
            Attribute att{getId()};
			att.setValue<float>(m_value);
			node.attributes.push_back(att);
		}

		float m_value;
	};

	//! Special class for the DATA_FTD structure for SIFA information
	class SifaFsDataItem : public FsDataItem
	{
	public:
		SifaFsDataItem(bool haupt, bool licht) : FsDataItem(Fs_Sifa), m_licht(licht), m_hauptschalter(haupt) {}

		virtual bool operator==(const FsDataItem& a) {
			if (getId() != a.getId())
				return false;

			const SifaFsDataItem* pA = dynamic_cast<const SifaFsDataItem*>(&a);
			if (pA && pA->m_licht == m_licht
				&& pA->m_hauptschalter == m_hauptschalter
				&& pA->m_hupewarning == m_hupewarning
				&& pA->m_hupebrems == m_hupebrems
				&& pA->m_storschalter == m_storschalter
				&& pA->m_luftabsper == m_luftabsper)
				return true;

			return false;
		}

		virtual void appendTo(Node& node) const
		{
            Node sNode{getId()};

            sNode.attributes.emplace_back(Attribute{1, uint8_t{'0'}});
            sNode.attributes.emplace_back(Attribute{2, uint8_t{m_licht}});
            sNode.attributes.emplace_back(Attribute{3, uint8_t{m_hupebrems ? 2 : m_hupewarning ? 1 : 0}});
            sNode.attributes.emplace_back(Attribute{4, uint8_t{m_hauptschalter + 1}});
            sNode.attributes.emplace_back(Attribute{5, uint8_t{m_storschalter + 1}});
            sNode.attributes.emplace_back(Attribute{6, uint8_t{m_luftabsper + 1}});

			node.nodes.push_back(sNode);
		}

		bool m_licht = false, m_hupewarning = false, m_hupebrems = false, m_hauptschalter = true, m_storschalter = true, m_luftabsper = true;
	};


	//! Parent class for a connection
	class Connection
	{
	public:
		/**
		* @brief Create connection which will communicate over socket
		* @param socket The socket - class does not take ownership of it
		*/
		Connection(Socket* socket) : m_socket(socket)
		{
		}

		virtual ~Connection()
		{
		}

		//! Receive a message
		bool receiveMessage(Node& dest) const;

		//! Send a message
		bool sendMessage(Node& src);

		//! Check if there is data read
		bool dataAvailable() { return m_socket->DataToRead(); }

	protected:
		Socket* m_socket;
	};

	//! Manages connection to a Zusi server
	class ClientConnection : public Connection
	{
	public:
		ClientConnection(Socket* socket) : Connection(socket)
		{
		}

		virtual ~ClientConnection()
		{
		}

		/**
		* @brief Set up connection to server
		* 
		* Sends the HELLO and NEEDED_DATA commands and processes the results
		*
		* @param client_id Null-terminated character array with an identification string for the client
		* @param fs_data Fuehrerstand Data ID's to subscribe to
		* @param prog_data Zusi program status ID's to subscribe to
		* @param bedienung Subscribe to input events if true
		* @return True on success
		*/
		bool connect(const char* client_id, const std::vector<FuehrerstandData>& fs_data, const std::vector<ProgData>& prog_data, bool bedienung);

		/** 
		* @brief Send an INPUT command 
		* @return True on success
		*/
		bool sendInput(Tastatur taster, TastaturKommand kommand, TastaturAktion aktion, int16_t position);

		//! Get the version string supplied by the server
		std::string getZusiVersion()
		{
			return m_zusiVersion;
		}

		//! Get the connection info string supplied by the server
		std::string getConnectionnfo()
		{
			return m_connectionInfo;
		}

	private:
		std::string m_zusiVersion;
		std::string m_connectionInfo;

	};

	//! A Zusi Server emulator
	class ServerConnection : public Connection
	{
	public:
		ServerConnection(Socket* socket) : Connection(socket), m_bedienung(false)
		{
		}

		virtual ~ServerConnection()
		{
		}

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
		std::string getClientVersion()
		{
			return m_clientVersion;
		}

		//! Get the connection info string supplied by the client
		std::string getClientName()
		{
			return m_clientName;
		}

	private:
		std::string m_clientVersion;
		std::string m_clientName;

		std::set<FuehrerstandData> m_fs_data;
		std::set<ProgData> m_prog_data;
		bool m_bedienung;

	};

}

