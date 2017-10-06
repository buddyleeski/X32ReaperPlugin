#pragma once
#ifndef X32CLIENT_HH
#define X32CLIENT_HH


#include "oscpkt.h"
#include "udp.h"

namespace x32 {

#define X32BUFLEN			1024  //Max length of buffer
#define X32UDPRETRYLIMIT	3		// Max number of retries
#define X32UDPTIMEOUTUSEC	500000  // UDP timeout in micro seconds

	//#define PORT 10023   //The port on which to listen for incoming data

	enum class X32LabelType
	{
		Channel,
		DCA,
		BUS,
		Matrix,
		EffectsReturn,
		AuxIn
	};

	enum class X32Route
	{
		INVALID = -1,
		AN1_8 = 0,
		AN9_16,
		AN17_24,
		AN25_32,
		A1_8,
		A9_16 = 5,
		A17_24,
		A25_32,
		A33_40,
		A41_48,
		B1_8 = 10,
		B9_16,
		B17_24,
		B25_32,
		B33_40,
		B41_48 = 15,
		CARD1_8,
		CARD9_16,
		CARD17_24,
		CARD25_32,
		OUT1_8 = 20,
		OUT9_16,
		P161_8,
		P169_16,
		AUX1_6,
		AUXIN1_6 = 25
	};

	struct RouteInfo
	{
		X32Route type;
		int channelNumber;
	};


	class X32Client
	{
	public:
		X32Client(char*);
		~X32Client();

		bool SendUdp(oscpkt::Message);
		void SetLabelName(X32LabelType, int, char *);
		void X32Client::SetLabelColor(X32LabelType type, int index, int colorIndex);
		void X32Client::SetLabelColorFromValue(X32LabelType type, int index, int colorIndex);
		int X32Client::GetLabelColor(X32LabelType type, int index);
		RouteInfo X32Client::GetCardOutputSource(int cardChannel);

		oscpkt::PacketReader* SendAndReceive(oscpkt::Message);
		oscpkt::PacketReader* ReceiveUdp();

	private:
		char* _x32IPAddress;
		int _x32Port;
		struct sockaddr_in _si_x32;
		int sock, slen = sizeof(_si_x32);
		char _udpBuf[X32BUFLEN];
		WSADATA wsa;
		void _InitializeUdp();
		void _CleanUpUdp();
		bool _x32IsConnected = false;
		int _udpRetryCount = 0;
		char* GetX32LabelCommandPath(X32LabelType, int, char *);
		int X32Client::CalculateInputChannelFromCard(X32Route type, int cardChannel);
	};

	void X32Client::SetLabelName(X32LabelType type, int index, char * name)
	{
		char* commandPath = GetX32LabelCommandPath(type, index, "name");
		if (commandPath == NULL)
			return;

		oscpkt::Message message(commandPath);
		message.pushStr(name);
		delete commandPath;

		SendUdp(message);
	}

	void X32Client::SetLabelColorFromValue(X32LabelType type, int index, int color)
	{
		int h = (color >> 24) & 0xFF;
		int blue = (color >> 16) & 0xFF;
		int green = (color >> 8) & 0xFF;
		int red = color & 0xFF;
		
		blue = (blue <= 127 ? 0 : 255);
		green = (green <= 85 ? 0 : (green >= 170 ? 255 : 127));
		red = (red <= 85 ? 0 : (red >= 170 ? 255 : 127));
		
		int rgb = 1;
		rgb = (rgb << 8) + blue;
		rgb = (rgb << 8) + green;
		rgb = (rgb << 8) + red;

		int colorIndex;

		if (rgb <= 16777344)
			colorIndex = 0;
		else if (rgb <= 16809984)
			colorIndex = 1;
		else if (rgb <= 16842624)
			colorIndex = 2;
		else if (rgb <= 25165824)
			colorIndex = 3;
		else if (rgb <= 33505280)
			colorIndex = 4;
		else if (rgb <= 33537983)
			colorIndex = 5;
		else if (rgb <= 33554367)
			colorIndex = 6;
		else // 33554431 
			colorIndex = 7;

		SetLabelColor(type, index, colorIndex);
	}

	void X32Client::SetLabelColor(X32LabelType type, int index, int colorIndex)
	{
		char* commandPath = GetX32LabelCommandPath(type, index, "color");
		if (commandPath == NULL)
			return;

		oscpkt::Message message(commandPath);
		message.pushInt32(colorIndex);
		delete commandPath;

		SendUdp(message);
	}

	static int getColorFromColorId(int colorId)
	{
		int red, green, blue;
		red = green = blue = 0;

		switch (colorId)
		{
			case 0:
				break;
			case 1:
				red = 255;
				break;
			case 2:
				green = 255;
				break;
			case 3:
				red = green = 255;
				break;
			case 4:
				blue = 255;
				break;
			case 5:
				red = blue = 255;
				green = 128;
				break;
			case 6:
				green = blue = 255;
				break;
			case 7:
				red = green = blue = 255;
				break;
			default:
				green = blue = 255;
				break;
		};

		int rgb = 1;
		rgb = (rgb << 8) + blue;
		rgb = (rgb << 8) + green;
		rgb = (rgb << 8) + red;

		return rgb;
	}

	int X32Client::GetLabelColor(X32LabelType type, int index)
	{
		char* commandPath = GetX32LabelCommandPath(type, index, "color");
		if (commandPath == NULL)
			return getColorFromColorId(0);

		oscpkt::Message message(commandPath);
		delete commandPath;

		oscpkt::PacketReader* reader = SendAndReceive(message);
		oscpkt::Message* responseMessage = reader->popMessage();
		
		int colorId;
		responseMessage->arg().popInt32(colorId);
		delete reader;

		return getColorFromColorId(colorId);
	}

	char* X32Client::GetX32LabelCommandPath(X32LabelType type, int index, char * commandName)
	{
		char* cmdBuffer = new char[30];
		char* prefix;
		int typeCountLimit;

		switch (type)
		{
		case X32LabelType::Channel:
			prefix = "ch";
			typeCountLimit = 32;
			break;
		case X32LabelType::AuxIn:
			prefix = "auxin";
			typeCountLimit = 8;
			break;
		case X32LabelType::DCA:
			prefix = "dca";
			typeCountLimit = 8;
			break;
		case X32LabelType::BUS:
			prefix = "bus";
			typeCountLimit = 16;
			break;
		case X32LabelType::EffectsReturn:
			prefix = "fxrtn";
			break;
		case X32LabelType::Matrix:
			prefix = "mtx";
			typeCountLimit = 6;
			break;
		default:
			return "";
			typeCountLimit = -1;
			break;
		}

		if (index > typeCountLimit)
			return NULL;

		if (type == X32LabelType::DCA)
			sprintf(cmdBuffer, "/%s/%d/config/%s", prefix, index, commandName);
		else
			sprintf(cmdBuffer, "/%s/%02d/config/%s", prefix, index, commandName);
		return cmdBuffer;
	}

	RouteInfo X32Client::GetCardOutputSource(int cardChannel)
	{
		RouteInfo route;
		route.type = X32Route::INVALID;

		char cardRouteCommand[30];
		if (cardChannel <= 0)
			return route;
		else if (cardChannel <= 8)
			sprintf(cardRouteCommand, "/config/routing/CARD/1-8");
		else if (cardChannel <= 16)
			sprintf(cardRouteCommand, "/config/routing/CARD/9-16");
		else if (cardChannel <= 24)
			sprintf(cardRouteCommand, "/config/routing/CARD/17-24");
		else if (cardChannel <= 32)
			sprintf(cardRouteCommand, "/config/routing/CARD/25-32");
		else
			return route;

		oscpkt::Message commandMessage(cardRouteCommand);
		oscpkt::PacketReader* response = SendAndReceive(commandMessage);
		if (response != NULL)
		{
			oscpkt::Message* responseMessage = response->popMessage();
			int typeValue;
			responseMessage->arg().popInt32(typeValue);
			route.type = (X32Route)typeValue;
			route.channelNumber = CalculateInputChannelFromCard(route.type, cardChannel);

			delete response;
		}

		return route;
	}

	int X32Client::CalculateInputChannelFromCard(X32Route type, int cardChannel)
	{
		int offset = cardChannel % 8;
		if (offset == 0)
			offset = 8;

		if (type <= X32Route::AN25_32)
			offset = ((int)type) * 8 + offset;
		else if (type <= X32Route::A41_48)
			offset = ((int)type - ((int)X32Route::A1_8)) * 8 + offset;
		else if (type <= X32Route::B41_48)
			offset = ((int)type - ((int)X32Route::B1_8)) * 8 + offset;
		else if (type <= X32Route::CARD25_32)
			offset = ((int)type - ((int)X32Route::CARD1_8)) * 8 + offset;
		else if (type <= X32Route::OUT9_16)
			offset = ((int)type - ((int)X32Route::OUT1_8)) * 8 + offset;
		else if (type <= X32Route::P169_16)
			offset = ((int)type - ((int)X32Route::P161_8)) * 8 + offset;

		return offset;

	}

	X32Client::X32Client(char* x32IPAddress)
	{
		_x32IPAddress = x32IPAddress;
		_x32Port = 10023;

		_InitializeUdp();
	}

	X32Client::~X32Client()
	{
		closesocket(sock);
		WSACleanup();
	}

	void X32Client::_InitializeUdp()
	{
		if (_x32IsConnected)
			_CleanUpUdp();

		//Initialise winsock
		printf("\nInitialising Winsock...");
		if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		{
			printf("Failed. Error Code : %d", WSAGetLastError());
			exit(EXIT_FAILURE);
		}
		printf("Initialised.\n");

		//create socket
		if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		{
			printf("socket() failed with error code : %d", WSAGetLastError());
			exit(EXIT_FAILURE);
		}

		//setup address structure
		memset((char *)&_si_x32, 0, sizeof(_si_x32));
		_si_x32.sin_family = AF_INET;
		_si_x32.sin_port = htons(_x32Port);
		_si_x32.sin_addr.S_un.S_addr = inet_addr(_x32IPAddress);

		u_long nonBlock = 1;
		int iResult = ioctlsocket(sock, FIONBIO, &nonBlock);
		if (iResult != NO_ERROR)
			printf("ioctlsocket failed with error: %ld\n", iResult);

		_x32IsConnected = true;

		return;
	}

	void X32Client::_CleanUpUdp()
	{
		_x32IsConnected = false;
		closesocket(sock);
		WSACleanup();
	}

	bool X32Client::SendUdp(oscpkt::Message message)
	{
		if (!_x32IsConnected)
			_InitializeUdp();

		oscpkt::PacketWriter pw;
		pw.addMessage(message);

		if (sendto(sock, pw.packetData(), pw.packetSize(), 0, (struct sockaddr *) &_si_x32, sizeof(_si_x32)) == SOCKET_ERROR)
		{

			printf("sendto() failed with error code : %d", WSAGetLastError());
			if (_udpRetryCount < X32UDPRETRYLIMIT)
			{
				_udpRetryCount++;
				_InitializeUdp();
				return SendUdp(message);
			}
			return false;
			//exit(EXIT_FAILURE);
		}

		_udpRetryCount = 0;
		return true;
	}

	oscpkt::PacketReader* X32Client::SendAndReceive(oscpkt::Message message)
	{
		SendUdp(message);

		oscpkt::PacketReader* reader;
		reader = ReceiveUdp();

		//reader = ReceiveUdp();
		if (reader == NULL)
		{
			if (_udpRetryCount < X32UDPRETRYLIMIT)
			{
				_udpRetryCount++;
				_InitializeUdp();
				return SendAndReceive(message);
			}

			return reader;
		}

		return reader;
	}

	oscpkt::PacketReader* X32Client::ReceiveUdp()
	{
		FD_SET readSet;
		memset(_udpBuf, '\0', X32BUFLEN);
		int responseSize;
		//try to receive some data, this is a blocking call

		timeval timeout = timeval();
		timeout.tv_sec = 0;
		timeout.tv_usec = 500000;

		int retries = 3;
		while (retries >= 0)
		{
			FD_ZERO(&readSet);
			FD_SET(sock, &readSet);

			int ret = select(sock, &readSet, NULL, NULL, &timeout);
			if (ret < 0)
				printf("timed out");

			if (FD_ISSET(sock, &readSet))
			{
				responseSize = recvfrom(sock, _udpBuf, X32BUFLEN, 0, (struct sockaddr *) &_si_x32, &slen);
				if (responseSize == SOCKET_ERROR)
				{
					printf("recvfrom() failed with error code : %d", WSAGetLastError());
					exit(EXIT_FAILURE);
				}

				return new oscpkt::PacketReader(_udpBuf, responseSize);
				//oscpkt::PacketReader reader(udpBuf, responseSize);
				//oscpkt::Message* response = reader.popMessage();
			}

			retries--;
		}

		return NULL;
	}
}

#endif