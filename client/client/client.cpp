#define WIN32_LEAN_AND_MEAN

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

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "5426"
#define TEST_PACKET_CONTENT "packet-type: 4-Receive\nrequest-content: from-index: 0\nmessage: Hello World!"
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
int func(SOCKET ConnectSocket)
{
	char recvbuf[DEFAULT_BUFLEN];
	int iResult;
	int recvbuflen = DEFAULT_BUFLEN;
	do {

		iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0)
		{
			printf("Bytes received: %d\n", iResult);
			printf("content: %s\n", recvbuf);
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
	onlineClient *first = new onlineClient("10.0.0.1", 1);
	onlineClient *second = new onlineClient("10.0.0.2", 2);
	onlineClientList.insert(std::make_pair(0, first));
	onlineClientList.insert(std::make_pair(1, second));
	std::cout << listCurrentAliveClient() << std::endl;
	using std::chrono::system_clock;

	std::chrono::duration<int, std::ratio<60 * 60 * 24> > one_day(1);

	system_clock::time_point today = system_clock::now();
	system_clock::time_point tomorrow = today + one_day;

	std::time_t tt;

	tt = system_clock::to_time_t(today);
	std::cout << "today is: " << ctime(&tt);

	tt = system_clock::to_time_t(tomorrow);
	std::cout << "tomorrow will be: " << ctime(&tt);

	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
	char sendbuf[DEFAULT_BUFLEN];;
	char recvbuf[DEFAULT_BUFLEN];
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
						printf("Please input the operation number:\n1.close the connect\n2.get the server time\n3.get the server name\n4.get the client list\n5.send a message\n6.exit");
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
						}
						else if (a == 2) {
							memcpy(sendbuf, TEST_PACKET_CONTENT, sizeof(TEST_PACKET_CONTENT));
							iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
							if (iResult == SOCKET_ERROR) {
								printf("send failed with error: %d\n", WSAGetLastError());
								closesocket(ConnectSocket);
								WSACleanup();
								break;
							}
							printf("Bytes Sent: %ld\n", iResult);
							continue;
						}
						else if (a == 3) {
							memcpy(sendbuf, "name", sizeof("name"));
							iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
							if (iResult == SOCKET_ERROR) {
								printf("send failed with error: %d\n", WSAGetLastError());
								closesocket(ConnectSocket);
								WSACleanup();
								break;
							}
							printf("Bytes Sent: %ld\n", iResult);
						}
						else if (a == 4) {
							memcpy(sendbuf, "list", sizeof("list"));
							iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
							if (iResult == SOCKET_ERROR) {
								printf("send failed with error: %d\n", WSAGetLastError());
								closesocket(ConnectSocket);
								WSACleanup();
								break;
							}
							printf("Bytes Sent: %ld\n", iResult);
						}
						else if (a == 5) {
							char message[255];
							printf("please input the message:\n");
							scanf("%s", message);
							memcpy(sendbuf, message, sizeof(message));
							iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
							if (iResult == SOCKET_ERROR) {
								printf("send failed with error: %d\n", WSAGetLastError());
								closesocket(ConnectSocket);
								WSACleanup();
								break;
							}
							printf("Bytes Sent: %ld\n", iResult);
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