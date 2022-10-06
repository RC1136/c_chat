#include <stdio.h>
#include <Ws2tcpip.h>
#include <windows.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <locale.h>

#define DEF_IP "0.0.0.0"
#define DEF_PORT "27015"
#define DEF_BUFFSIZE (4096)
#define PORTSTRLEN (6)
#define ESC "\x1b"
#define DEF_COL ESC"[0m"
#define RED_COL ESC"[31m"

char _buf[DEF_BUFFSIZE] = "";

typedef struct {
	SOCKET socket;
	HANDLE thread;
	DWORD threadID;
	char* buffer;
	char name[64];
	CHAR ip[INET_ADDRSTRLEN];
	int id;
}ClientInfo;
ClientInfo clients[256] = {INVALID_SOCKET, NULL, 0, _buf, "admin", 0};




const char JOINMES[] = " has joined the chat";

//#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib,"WS2_32")		//error LNK2019 майже у всіх функціях

void inputaddr(char* ip, char* strport)
{
	printf("input ip ("DEF_IP") : ");
	int i = 0;

	while ((ip[i] = getchar()) != '\n') {
		if (++i == INET_ADDRSTRLEN) {
			while (getchar() != '\n');
			printf("invalid input...\n");
			printf("input ip ("DEF_IP") : ");
			i = 0;
			continue;
		}

	}
	ip[i] = '\0';

	if (i == 0)
		strcpy_s(ip, INET_ADDRSTRLEN, DEF_IP);
	else
		i = 0;

	printf("input port ("DEF_PORT"): ");
	while ((strport[i] = getchar()) != '\n') {
		if (++i == PORTSTRLEN) {
			while (getchar() != '\n');
			printf("invalid input...\n");
			printf("input port ("DEF_PORT"): ");
			i = 0;
			continue;
		}

	}
	strport[i] = '\0';

	if (i == 0)
		strcpy_s(strport, PORTSTRLEN, DEF_PORT);

}

SOCKET GetListener(const char* addr, const char* strport)
{
	int res;
	/*
	//char* strport = (char*)malloc(6 * sizeof(char));
	char strport[6] = "";
	//if (strport == NULL)
	//	return GetListener(addr, port);
	_itoa_s((int)port, strport, 6, 10);
	*/

	WSADATA wsaData;
	SOCKET ListenSocket = INVALID_SOCKET;
	struct addrinfo* result = NULL;
	struct addrinfo hints;

	//Ініціалізую сокет
	res = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (res != 0) {
		printf("WSAStartup failed (%d)\n", res);
		return INVALID_SOCKET;
	}

	ZeroMemory(&hints, sizeof(hints));
	//memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	//Отримуєм адресу серверу і порт
	res = getaddrinfo(	//https://docs.microsoft.com/en-us/windows/win32/api/ws2tcpip/nf-ws2tcpip-getaddrinfo
		NULL,				//Адреса
		strport,			//Порт
		&hints,				//Рекомендації по сокету (що ми хочем)
		&result				//Інформація по сокету (що ми отримали)
	);
	if (res != 0) {
		printf("getaddrinfo failed (%d)\n", res);
		WSACleanup();
		return INVALID_SOCKET;
	}

	//Створюю сокет для з'єднання з сервером
	ListenSocket = socket(//https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-socket
		result->ai_family,
		result->ai_socktype,
		result->ai_protocol
	);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed (%d)\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return INVALID_SOCKET;
	}

	//Встановлюю прослуховуючий (Listening) сокет
	res = bind(//https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-bind
		ListenSocket,			//Дескриптор сокета
		result->ai_addr,		//Адреса (і всяке таке)
		(int)result->ai_addrlen	//Довжина, в байтах, значення імені (костиль - інфа сотка)
	);
	if (res == SOCKET_ERROR) {
		printf("bind failed (%d)\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return INVALID_SOCKET;
	}
	freeaddrinfo(result);
	//free(strport);

	return ListenSocket;
}

SOCKET AcceptClient(SOCKET ListenSocket)
{
	printf("Waiting for client...\n");
	int res;
	SOCKET ClientSocket = INVALID_SOCKET;
	res = listen(//https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-listen
		ListenSocket,	//Сокет
		SOMAXCONN		//Довжина черги
	);
	if (res == SOCKET_ERROR) {
		printf("listen failed (%d)\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return INVALID_SOCKET;
	}
	struct sockaddr_in client_addrinfo;
	ZeroMemory(&client_addrinfo, sizeof(client_addrinfo));
	int tmp = sizeof(client_addrinfo);

	//Приймаю з'єднання
	ClientSocket = accept(//https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-accept
		ListenSocket,
		(struct sockaddr*)&client_addrinfo,
		&tmp
	);
	if (ClientSocket == INVALID_SOCKET) {
		printf("accept failed (%d)\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return INVALID_SOCKET;
	}

	//getpeername(ClientSocket, (struct sockaddr*)&client_addrinfo, &tmp);

	char cliIP[INET_ADDRSTRLEN];
	inet_ntop(client_addrinfo.sin_family, &(client_addrinfo.sin_addr), cliIP, INET_ADDRSTRLEN);
	printf("Client's ip is: %s\n", cliIP); //inet_ntoa(client_addrinfo.sin_addr)



	return ClientSocket;
}

int SendMes(int length, ClientInfo client)
{
	//char *tmp = malloc(128);
	char tmp[128] = "";
	//char *tm = malloc(64);
	char tm[64];
	time_t t = time(NULL);
	ctime_s(tm, 63, &t);
	tmp[0] = '[';
	_itoa_s(client.id, tmp + 1, sizeof(tmp) - 1, 10);
	strcat_s(tmp, sizeof(tmp) - 42, "]:\n");
	int namelen = strlen(client.name), tmplen = strlen(tmp), tmlen = strlen(tm);
	int meslen = length + namelen + tmplen + tmlen;
	int res = 0;
	for (int i = 1; i < (sizeof(clients) / sizeof(*clients)); i++) {
		if (clients[i].id < 0)
			continue;
		res = send(clients[i].socket, (void*)&meslen, sizeof(meslen), 0);
		res = send(clients[i].socket, tm, tmlen, 0);
		res = send(clients[i].socket, client.name, namelen, 0);
		res = send(clients[i].socket, tmp, tmplen, 0);
		res = send(clients[i].socket, client.buffer, length, 0);
	}

	//free(tmp);
	return 0;
}

int SendAdminMes(char* mes, int length)
{
	
	int res = 0, meslen = 0;

	char buff[DEF_BUFFSIZE] = "";
	strcpy_s(buff, sizeof(RED_COL), RED_COL);
	meslen = sizeof(RED_COL);
	strcpy_s(buff + meslen - 1, length, mes);
	meslen += length - 1;
	strcpy_s(buff + meslen - 1, sizeof(DEF_COL), DEF_COL);
	meslen += sizeof(DEF_COL) - 1;

	for (int i = 1; i < (sizeof(clients) / sizeof(*clients)); i++) {
		if (clients[i].id < 1)
			continue;
		res = send(clients[i].socket, (void*)&meslen, sizeof(meslen), 0);
		res = send(clients[i].socket, buff, meslen, 0);
	}

	
	return 0;
}

DWORD WINAPI ClientHandler(LPVOID lpparam)
{
	int bytes = DEF_BUFFSIZE, res = 0;
	ClientInfo* client = lpparam;
	client->buffer = malloc(DEF_BUFFSIZE);
	if (client->buffer == NULL) return 0;


	res = getpeername(client->socket, (struct sockaddr*)(client->buffer), &bytes); 
	
	if (res == -1)
		printf("%d\n", WSAGetLastError());
	else
		inet_ntop((*(struct sockaddr_in*)(client->buffer)).sin_family, &(*(struct sockaddr_in*)(client->buffer)).sin_addr, client->ip, INET_ADDRSTRLEN);
		//strcpy_s(client->ip, bytes, (*(struct sockaddr*)client->buffer).sa_data);


	res = recv(client->socket, (void*)&bytes, sizeof(bytes), MSG_WAITALL);
	res = recv(client->socket, client->name, (int)bytes, MSG_WAITALL);

	strcpy_s(client->buffer, bytes, client->name);
	strcpy_s(client->buffer + bytes - 1, sizeof(JOINMES), JOINMES);
	bytes += sizeof(JOINMES) - 1;
	SendAdminMes(client->buffer, bytes);


	while (1) {
		if ((res = recv(client->socket, (void*)&bytes, sizeof(bytes), MSG_WAITALL)) < 1)
			break;
		if ((res = recv(client->socket, client->buffer, (int)bytes, MSG_WAITALL)) < 1)
			break;
		SendMes(bytes, *client);

	}
	printf("%s[%d] (%s) closed connection with code %#X\n", client->name, client->id, client->ip, WSAGetLastError());

	free(client->buffer);
	closesocket(client->socket);
	return 0;
}


int main(int argc, char* argv[])
{
	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);
	

	for (int i = 1; i < sizeof(clients) / sizeof(*clients); i++) {
		clients[i].id = -1;
	}
	char ip[INET_ADDRSTRLEN] = "";
	char strport[6] = "";

	if (argc > 2) {
		strcpy_s(ip, sizeof(ip), argv[1]);
		strcpy_s(strport, sizeof(strport), argv[2]);
		//port = atoi(argv[2]);
	}
	else if (argc > 1) {
		strcpy_s(strport, sizeof(strport), argv[1]);
		//port = atoi(argv[1]);
	}
	else {
		inputaddr(ip, strport);
		/*
		printf("input ip (0.0.0.0): ");
		int i = 0;
		
		while ((ip[i] = getchar()) != '\n') {
			if (++i == sizeof(ip)){
				while (getchar() != '\n');
				printf("invalid input...\n");
				printf("input ip (0.0.0.0): ");
				i = 0;
				continue;
			}
				
		}
		ip[i] = '\0';

		printf("input port (" DEF_PORT "): ");
		while ((strport[i] = getchar()) != '\n') {
			if (++i == sizeof(strport)) {
				while (getchar() != '\n');
				printf("invalid input...\n");
				printf("input port (" DEF_PORT "): ");
				i = 0;
				continue;
			}

		}
		strport[i] = '\0';

		if (i == 0)
			strcpy_s(strport, sizeof(strport), DEF_PORT);
		*/
	}



	SOCKET ListenSocket = INVALID_SOCKET;
	ListenSocket = GetListener(ip, strport);
	SOCKET ClientSocket = INVALID_SOCKET;
	int id = 1;
	//Чекаю з'єднання
	while (1) {
		ClientSocket = AcceptClient(ListenSocket);
		if (ClientSocket == INVALID_SOCKET) {
			continue;
		}
		for (id = 1; clients[id].thread != NULL; id++) {
			if (WaitForSingleObject(clients[id].thread, 0) == WAIT_OBJECT_0)
				break;
		}
		clients[id].id = id;
		clients[id].socket = ClientSocket;
		clients[id].thread = CreateThread(NULL, 0, ClientHandler, clients + id, 0, &clients->threadID);
		
	}
	WSACleanup();
	closesocket(ListenSocket);


	while (getchar() != '\n');
	return 0;
}