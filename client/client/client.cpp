#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

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
#include <map>
#include <mutex>
#include <ctime>
#include <ratio>
#include <chrono>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "5426"
#define TEST_PACKET_CONTENT "packet-type: 4-Receive\nrequest-content: from-index: 0\nmessage: Hello World!"
std::mutex mtx_time;
std::mutex mtx_name;
std::mutex mtx_list;
std::mutex mtx_message;
std::mutex mtx_output;
std::condition_variable Time;
std::condition_variable name;
std::condition_variable list;
std::condition_variable message;
int func(SOCKET ConnectSocket)
{
	char recvbuf[DEFAULT_BUFLEN];
	int iResult;
	int recvbuflen = DEFAULT_BUFLEN;
	do {
		ZeroMemory(recvbuf, DEFAULT_BUFLEN);
		iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0)
		{
			std::string recvPacket = recvbuf;
			std::string packetType;
			std::string responseContent;
			size_t pos = 0;
			pos = recvPacket.find(" ") + 1;
			size_t nextPos = 0;
			//get packet type
			nextPos = recvPacket.find("\n", pos);
			packetType = recvPacket.substr(pos, nextPos - pos);
			//get request-content type
			pos = recvPacket.find("response-content:");
			nextPos = recvPacket.find(" ", pos);
			if (nextPos == -1)
				responseContent = "";
			else
				responseContent = recvPacket.substr(nextPos + 1);
			if (packetType.compare("1") == 0)
			{
				mtx_output.lock();
				std::cout << responseContent << std::endl;
				mtx_output.unlock();
				Time.notify_one();
			}
			else if (packetType.compare("2") == 0)
			{
				mtx_output.lock();
				std::cout << responseContent << std::endl;
				mtx_output.unlock();
				name.notify_one();
			}
			else if (packetType.compare("3") == 0)
			{
				mtx_output.lock();
				std::cout << responseContent << std::endl;
				mtx_output.unlock();
				list.notify_one();
			}
			else if (packetType.compare("4-S") == 0)
			{
				int requestIndex;
				pos = recvPacket.find("request-index:");
				nextPos = recvPacket.find(" ", pos) + 1;
				pos = recvPacket.find("\n", nextPos);
				requestIndex = stoi(recvPacket.substr(nextPos, pos - nextPos));
				
				pos = recvPacket.find("message:");
				nextPos = recvPacket.find(" ", pos) + 1;
				std::string sendMessage = recvPacket.substr(nextPos);

				mtx_output.lock();
				std::cout << "the message has been sent to" << std::to_string(requestIndex) << std::endl;
				mtx_output.unlock();
				message.notify_one();
			}
			else if (packetType.compare("4-F-not-exist") == 0)
			{
				mtx_output.lock();
				std::cout << "This client doesn't exist or it has logged out" << std::endl;
				mtx_output.unlock();
				message.notify_one();
			}
			else if (packetType.compare("message") == 0)
			{
				int fromIndex;
				pos = recvPacket.find("from: ");
				nextPos = recvPacket.find(" ", pos) + 1;
				pos = recvPacket.find("\n", nextPos);
				fromIndex = stoi(recvPacket.substr(nextPos, pos - nextPos));

				std::string receiveMessage = recvPacket.substr(pos + 1);

				mtx_output.lock();
				std::cout << "Message from " << std::to_string(fromIndex) << std::endl;
				std::cout << receiveMessage << std::endl;
				mtx_output.unlock();
				
				std::string sendMessage = "packet-type: 4-Receive\nrequest-content: from-index: ";
				sendMessage += std::to_string(fromIndex);
				sendMessage += '\n';
				sendMessage += "message: ";
				sendMessage += receiveMessage;
				send(ConnectSocket, sendMessage.c_str(), sendMessage.size(), 0);

			}
		}
		else if (iResult == 0)
			printf("Connection closed\n");
		else
			printf("recv failed with error: %d\n", WSAGetLastError());

	} while (iResult > 0);
	return 0;
}
int __cdecl main(void)
{
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
	char sendbuf[DEFAULT_BUFLEN];
	int iResult;
	int recvbuflen = DEFAULT_BUFLEN;
	char ip[DEFAULT_BUFLEN];;
	ZeroMemory(sendbuf, DEFAULT_BUFLEN);
	while (1) {
		printf("Please choose the operation number:\n 1.connect server 2.exit\n");
		int c;
		scanf("%d", &c);
		if (c == 2) {
			return 0;
		}
		else if (c == 1) {
			printf("please input the server ip:\n");
			scanf("%s", ip);
			// Validate the parameters
			if (ip) {
				printf("usage: %s server-name\n", ip);
			}
			iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
			if (iResult != 0) {
				printf("WSAStartup failed with error: %d\n", iResult);
				system("pause");
				continue;
			}
			ZeroMemory(&hints, sizeof(hints));
			hints.ai_family = AF_UNSPEC;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_protocol = IPPROTO_TCP;

			// Resolve the server address and port
			iResult = getaddrinfo(ip, DEFAULT_PORT, &hints, &result);
			if (iResult != 0) {
				printf("getaddrinfo failed with error: %d\n", iResult);
				WSACleanup();
				continue;
			}
			int flag = 0;
			for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
				// Create a SOCKET for connecting to server
				ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
					ptr->ai_protocol);
				if (ConnectSocket == INVALID_SOCKET) {
					printf("socket failed with error: %ld\n", WSAGetLastError());
					WSACleanup();
					flag = 1;
					break;
				}
				// Connect to server.
				iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
				if (iResult == SOCKET_ERROR) {
					closesocket(ConnectSocket);
					ConnectSocket = INVALID_SOCKET;
					continue;
				}
				break;
			}
			if (flag == 1) {
				continue;
			}
			else {
				freeaddrinfo(result);
				if (ConnectSocket == INVALID_SOCKET) {
					printf("Unable to connect to server!\n");
					WSACleanup();
					continue;
				}
				else {
					printf("Connection succeed!\n");
					std::thread(func, std::move(ConnectSocket)).detach();
					while (1) {
						printf("Please input the operation number:\n1.close the connect\n2.get the server time\n3.get the server name\n4.get the client list\n5.send a message\n6.exit\n");
						int a;
						scanf("%d", &a);
						if (a == 1) {
							iResult = shutdown(ConnectSocket, SD_SEND);
							if (iResult == SOCKET_ERROR) {
								printf("shutdown failed with error: %d\n", WSAGetLastError());
								closesocket(ConnectSocket);
								WSACleanup();
								break;
							}
							break;
						}
						else if (a == 2) {
							std::string packet = "packet-type: 1\nrequest-content:";
							memcpy(sendbuf, packet.c_str(), packet.size());
							iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
							if (iResult == SOCKET_ERROR) {
								printf("send failed with error: %d\n", WSAGetLastError());
								closesocket(ConnectSocket);
								WSACleanup();
								break;
							}
							std::unique_lock<std::mutex> lck(mtx_time);
							Time.wait(lck);
							continue;
						}
						else if (a == 3) {
							std::string packet = "packet-type: 2\nrequest-content:";
							memcpy(sendbuf, packet.c_str(), packet.size());
							iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
							if (iResult == SOCKET_ERROR) {
								printf("send failed with error: %d\n", WSAGetLastError());
								closesocket(ConnectSocket);
								WSACleanup();
								break;
							}
							std::unique_lock<std::mutex> lck(mtx_name);
							name.wait(lck);
							continue;
						}
						else if (a == 4) {
							std::string packet = "packet-type: 3\nrequest-content:";
							memcpy(sendbuf, packet.c_str(), packet.size()); 
							iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
							if (iResult == SOCKET_ERROR) {
								printf("send failed with error: %d\n", WSAGetLastError());
								closesocket(ConnectSocket);
								WSACleanup();
								break;
							}
							std::unique_lock<std::mutex> lck(mtx_list);
							list.wait(lck);
							continue;
						}
						else if (a == 5) {
							std::string sendMessage;
							int dstIndex = 0;
							std::cout << "please input the dst index:" << std::endl;
							std::cin >> dstIndex;
							std::cout << "please input the message:" << std::endl;
							std::cin >> sendMessage;
							std::string packet = "packet-type: 4-Send\nrequest-content: request-index: ";
							packet += std::to_string(dstIndex);
							packet += '\n';
							packet += "message: ";
							packet += sendMessage;
							ZeroMemory(sendbuf, DEFAULT_BUFLEN);
							memcpy(sendbuf, packet.c_str(), packet.size());
							iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
							if (iResult == SOCKET_ERROR) {
								printf("send failed with error: %d\n", WSAGetLastError());
								closesocket(ConnectSocket);
								WSACleanup();
								break;
							}
							std::unique_lock<std::mutex> lck(mtx_message);
							message.wait(lck);
							continue;
						}
						else if (a == 6) {
							printf("Good bye!\n");
							closesocket(ConnectSocket);
							WSACleanup();
							return 0;
						}
						else {
							continue;
						}
					}
					continue;
				}
			}
		}
		else {
			continue;
		}
	}
}