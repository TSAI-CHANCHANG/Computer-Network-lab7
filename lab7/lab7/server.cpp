#undef UNICODE
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <thread>
#include <iostream>
#include <streambuf> 
#include <fstream>
#include <string>
#include <thread>
#include <map>
#include <mutex>
#include <ctime>
#include <ratio>
#include <chrono>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define SERVER_NAME "×µÃû¤Þ¤·¤í"
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "5426"

#define DEBUG
class onlineClient
{
private:
	char *IP;
	int port;
public:
	onlineClient(char *currentIP, int currentPort) : IP(currentIP), port(currentPort) {}
	~onlineClient() {}
	char *getIP()
	{
		return IP;
	}
	int getPort()
	{
		return port;
	}
};
std::map<int, onlineClient*> onlineClientList;
std::mutex mt;
int currentFreeIndex = 0;
bool isAlive(char *IP, int port);
std::string listCurrentAliveClient();
std::string dealWithReceivePacket(char recvbuf[], int recvbuflen)
{
	std::string recvPacket = recvbuf;
	std::string packetType;
	std::string requestContent;
	std::string response;
	int pos = 0;
	pos = recvPacket.find(" ") + 1;
	int nextPos = 0;
	//get packet type
	nextPos = recvPacket.find("\n", pos);
	packetType = recvPacket.substr(pos, nextPos - pos);
	//get request-content type
	pos = recvPacket.find("request-content:");
	nextPos = recvPacket.find(" ", pos);
	if (nextPos == -1)
		requestContent = "";
	else
		requestContent = recvPacket.substr(nextPos + 1);
	//now we can decide what we send
	if (packetType.compare("1") == 0)//get time
	{
		response += "packet-type: 1\nresponse-content: ";
		using std::chrono::system_clock;
		system_clock::time_point today = system_clock::now();
		std::time_t tt;
		tt = system_clock::to_time_t(today);
		response += ctime(&tt);
	}
	else if (packetType.compare("2") == 0)//get name
	{
		response += "packet-type: 2\nresponse-content: ";
		response += SERVER_NAME;
	}
	else if (packetType.compare("3") == 0)//get list
	{
		response += "packet-type: 3\nresponse-content: ";
		response += listCurrentAliveClient();
	}
	else if (packetType.compare("4") == 0)//get message
	{
		response += "packet-type: 4\nresponse-content: ";
	}
	else//wrong
	{
		response += "packet-type: 5\nresponse-content: ";
	}
	return response;
}
bool isAlive(char *IP, int port)
{
	bool flag = false;
	mt.lock();
	std::map<int, onlineClient*>::iterator iter;
	iter = onlineClientList.begin();
	while (iter != onlineClientList.end())
	{
		if (strcmp(iter->second->getIP(), IP) == 0 &&
			iter->second->getPort() == port)
		{
			flag = true;
			break;
		}
		iter++;
	}
	mt.unlock();

	if (flag)
		return true;
	else
		return false;
}
std::string listCurrentAliveClient()
{
	std::string content;

	mt.lock();
	std::map<int, onlineClient*>::iterator iter;
	iter = onlineClientList.begin();
	while (iter != onlineClientList.end())
	{
		content += std::to_string(iter->first);
		content += ' ';
		content += iter->second->getIP();
		content += ' ';
		content += std::to_string(iter->second->getPort());
		content += '\n';
		iter++;
	}
	mt.unlock();

	return content;
}
int func(SOCKET ClientSocket)
{
	int iSendResult;
	char recvbuf[DEFAULT_BUFLEN] = "\0";
	char sendbuf[DEFAULT_BUFLEN] = "\0";
	int recvbuflen = DEFAULT_BUFLEN;
	bool isShutdown = false;
	int iResult = 0;
	char *IP = NULL;
	int port = 0;
	//get information about this client
	SOCKADDR_IN clientInfo = { 0 };
	int addrsize = sizeof(clientInfo);
	//get current ip, port
	getpeername(ClientSocket, (struct sockaddr*)&clientInfo, &addrsize);
	IP = inet_ntoa(clientInfo.sin_addr);
	port = clientInfo.sin_port;
#ifdef DEBUG
	printf("port: %d\n", clientInfo.sin_port);
	printf("ip:%s\n", IP);
#endif
	//add it to the list
	onlineClient *thisClient = new onlineClient(IP, port);
	int thisIndex = 0;
	mt.lock();
	thisIndex = currentFreeIndex;
	onlineClientList.insert(std::make_pair(currentFreeIndex, thisClient));
	currentFreeIndex++;
	mt.unlock();
	//then analysis received packet
	while (!isShutdown)
	{
		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		printf("port: %d\n", clientInfo.sin_port);
		printf("ip:%s\n", IP);
		printf("Bytes received: %d\n", iResult);
		printf("received content: %s\n", recvbuf);
		if (iResult > 0) {
			std::string content = dealWithReceivePacket(recvbuf, recvbuflen);
			ZeroMemory(sendbuf, DEFAULT_BUFLEN);
			memcpy(sendbuf, content.c_str(), content.size());
			// Echo the buffer back to the sender
			iSendResult = send(ClientSocket, sendbuf, DEFAULT_BUFLEN, 0);
			if (iSendResult == SOCKET_ERROR) {
				printf("send failed with error: %d\n", WSAGetLastError());
				closesocket(ClientSocket);
				WSACleanup();
				return 1;
			}
			printf("------------------------------------\n");
			printf("Bytes sent: %d\n", iSendResult);
			printf("send content: %s\n", sendbuf);
		}
		else if (iResult == 0)
		{
			printf("Connection closing...\n");
			isShutdown = true;
		}
		else {
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			mt.lock();
			onlineClientList.erase(thisIndex);
			mt.unlock();

			delete thisClient;
			WSACleanup();
			return 1;
		}
	}
	//now shutdown the socket
	iResult = shutdown(ClientSocket, SD_SEND);
	closesocket(ClientSocket);
	//delete the client from the list
	mt.lock();
	onlineClientList.erase(thisIndex);
	mt.unlock();

	delete thisClient;
	return 0;
}

int __cdecl main(void)
{
	WSADATA wsaData;
	int iResult;

	SOCKET ListenSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	do {
		iResult = listen(ListenSocket, SOMAXCONN);
		if (iResult == SOCKET_ERROR) {
			printf("listen failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		SOCKADDR_IN clientInfo = { 0 };
		int addrsize = sizeof(clientInfo);
		// Accept a client socket
		SOCKET ClientSocket = INVALID_SOCKET;
		ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET) {
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		std::thread(func, std::move(ClientSocket)).detach();

	} while (true);

	// No longer need server socket
	closesocket(ListenSocket);
	// shutdown the connection since we're done
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	// cleanup
	WSACleanup();

	return 0;
}