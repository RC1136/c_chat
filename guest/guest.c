#include <stdio.h>
#include <Ws2tcpip.h>
#include <windows.h>
#include <string.h>
#include <assert.h>
#include <locale.h>
#include <conio.h>

#define DEF_IP "127.0.0.1"
#define DEF_PORT "27015"
#define DEF_BUFFSIZE (4096)
#define PORTSTRLEN (6)

#define ESC "\x1b"
#define TUUP(x) ESC"["x"F"
HANDLE mutex;

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
	printf("input port (" DEF_PORT "): ");
	while ((strport[i] = getchar()) != '\n') {
		if (++i == PORTSTRLEN) {
			while (getchar() != '\n');
			printf("invalid input...\n");
			printf("input port (" DEF_PORT "): ");
			i = 0;
			continue;
		}

	}
	strport[i] = '\0';

	if (i == 0)
		strcpy_s(strport, PORTSTRLEN, DEF_PORT);

}

SOCKET ConnectServer(char* ip, char* strport)
{
	/*
	char* strport = (char*)malloc(6 * sizeof(char));
	if (strport == NULL)
		return 1;
	_itoa_s((int)port, strport, 6, 10);
	*/
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo* result = NULL,
		hints;
	int res;

	//ініціалізую сокет
	res = WSAStartup(//https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-wsastartup
		MAKEWORD(2, 2),
		&wsaData
	);
	if (res != 0) {
		printf("WSAStartup failed (%d)\n", res);
		return INVALID_SOCKET;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	//отримую адресу і порт
	res = getaddrinfo(//https://docs.microsoft.com/en-us/windows/win32/api/ws2tcpip/nf-ws2tcpip-getaddrinfo
		ip,				//адреса
		strport,		//порт
		&hints,
		&result
	);
	if (res != 0) {
		printf("getaddrinfo failed (%d)\n", res);
		WSACleanup();
		return INVALID_SOCKET;
	}
	//створюю сокет для з'єднання з сервером
	ConnectSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ConnectSocket == INVALID_SOCKET) {
		printf("socket failed (%ld)\n", WSAGetLastError());
		WSACleanup();
		return INVALID_SOCKET;
	}
	//з'єднююсь з сервером
	res = connect(ConnectSocket, result->ai_addr, (int)result->ai_addrlen);
	if (res == SOCKET_ERROR) {
		closesocket(ConnectSocket);
		return INVALID_SOCKET;
	}

	freeaddrinfo(result);
	if (ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return INVALID_SOCKET;
	}

	return ConnectSocket;
}

void SetupConsole()
{
	setlocale(LC_ALL, "ukr");
	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	//printf(SetConsoleTextAttribute(hOut, ENABLE_VIRTUAL_TERMINAL_PROCESSING) ? "true" : "false");
	DWORD consoleMode;
	GetConsoleMode(hOut, &consoleMode);
	consoleMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	consoleMode |= DISABLE_NEWLINE_AUTO_RETURN;
	if(!SetConsoleMode(hOut, consoleMode))
		printf("failed to set console mod (%#X)\n", GetLastError());
	//printf(ESC"[0m");
	//SetConsoleScreenBufferInfoEx(hOut, );
	SetConsoleTitleA("C_Chat2000");
	//CloseHandle(hOut);
}


DWORD WINAPI Receiver(LPVOID lpparam) 
{
	SOCKET Server = *(SOCKET*)lpparam;
	//char* buffer = malloc(DEF_BUFFSIZE);
	char buffer[DEF_BUFFSIZE];
	int bytestoreceive = 0, res = 0;
	while (1) {

		res = recv(Server, (void*)&bytestoreceive, sizeof(bytestoreceive), MSG_WAITALL);
		res = recv(Server, buffer, bytestoreceive, MSG_WAITALL);
		if (res < 1)
			goto err;
		WaitForSingleObject(mutex, INFINITE);
		printf(TUUP("2") "%s" "\n\n\n\n", buffer);
		//printf("\n%s\n", buffer);
		ReleaseMutex(mutex);

	}
	//free(buffer);
	return 0;

err:
	//free(buffer);
	printf("Receiver crushed.\nGetLastError: %#X.\nWSAGetLastError: %#X\n", GetLastError(), WSAGetLastError());
	return -1;
}

DWORD WINAPI Sender(LPVOID lpparam)
{
	WaitForSingleObject(mutex, INFINITE);
	SOCKET Server = *(SOCKET*)lpparam;
	//char* buffer = malloc(DEF_BUFFSIZE);
	char buffer[DEF_BUFFSIZE] = "";

	int bytestosend = 0, res = 0;

	printf("Input your name\n");
	do{
		while ((buffer[bytestosend] = getchar()) != '\n')
			bytestosend++;
	} while (bytestosend < 1);
	buffer[bytestosend++] = '\0';
	putchar('\n'); putchar('\n'); putchar('\n');
	ReleaseMutex(mutex);
	res = send(Server, (void*)&bytestosend, sizeof(bytestosend), 0);
	res = send(Server, buffer, bytestosend, 0);
	if (res < 1)
		goto err;

	while (1) {
		bytestosend = 0;
		//while (getchar() != '\n');
		while (!_kbhit())
			Sleep(1);
		WaitForSingleObject(mutex, INFINITE);

		while ((buffer[bytestosend] = getchar()) != '\n')
			bytestosend++;
		buffer[bytestosend++] = '\0';
		printf(TUUP("1"));
		if (bytestosend <= 1) {
			//printf("\x1b[1F");
			ReleaseMutex(mutex);
			continue;
		}

		//printf("%.*s", bytestosend, "\r");
		ReleaseMutex(mutex);
		res = send(Server, (void*)&bytestosend, sizeof(bytestosend), 0);
		res = send(Server, buffer, bytestosend, 0);
		if (res < 1)
			goto err;

	}

	//free(buffer);
	return 0;

err:
	//free(buffer);
	printf("Sender crushed.\nGetLastError: %#X.\nWSAGetLastError: %#X\n", GetLastError(), WSAGetLastError());
	return -1;
}

int main(int argc, char* argv[])
{
	SetupConsole();

	char ip[INET_ADDRSTRLEN] = "";
	char strport[PORTSTRLEN] = "";

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

	}
	SOCKET ConnectSocket;
	do {
		ConnectSocket = ConnectServer(ip, strport);
		printf("Wait timed out...\n");
	} while (ConnectSocket == INVALID_SOCKET);
	printf("Connected!\n");

	/*
	short port = DEF_PORT;
	char* ip = argc > 1 ? argv[1] : "127.0.0.1";	//Параметром передаю IP адресу
	SOCKET ConnectSocket;
	do {
		ConnectSocket = ConnectServer(ip, (int)port);
	} while (ConnectSocket == INVALID_SOCKET);
	*/
	

	HANDLE treceiver, tsender;
	int idreceiver = 0, idsender = 0;
	mutex = CreateMutex(NULL, FALSE, NULL);
	tsender = CreateThread(NULL, 0, Sender, &ConnectSocket, 0, &idsender);
	treceiver = CreateThread(NULL, 0, Receiver, &ConnectSocket, 0, &idreceiver);
	
	HANDLE threads[] = {treceiver, tsender};
	WaitForMultipleObjects(2, threads, FALSE, INFINITE);
	CloseHandle(mutex);
	
	CloseHandle(treceiver);
	CloseHandle(tsender);
	closesocket(ConnectSocket);
	WSACleanup();
	while (getchar() != '\n');
	return 0;
}