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
#include <vector>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "5426"

int func(SOCKET ClientSocket)
{
	int iSendResult;
	char recvbuf[DEFAULT_BUFLEN] = "\0";
	char sendbuf[DEFAULT_BUFLEN] = "\0";
	int recvbuflen = DEFAULT_BUFLEN;
	bool isShutdown = false;
	int iResult = 0;
	while (!isShutdown)
	{
		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		printf("Bytes received: %d\n", iResult);
		printf("received content: %s\n", recvbuf);
		if (iResult > 0) {
			memcpy(sendbuf, "114514", sizeof("114514"));
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
			WSACleanup();
			return 1;
		}
	}
	
	iResult = shutdown(ClientSocket, SD_SEND);
	closesocket(ClientSocket);
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
		ClientSocket = accept(ListenSocket, (struct sockaddr*)&clientInfo, &addrsize);
		if (ClientSocket == INVALID_SOCKET) {
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}
		//get current ip
		getpeername(ClientSocket, (struct sockaddr*)&clientInfo, &addrsize);
		char *ip = inet_ntoa(clientInfo.sin_addr);
		printf("port: %d", clientInfo.sin_port);

		printf("ip:%s", ip);

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