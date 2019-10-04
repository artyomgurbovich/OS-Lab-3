// OS Lab 3.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "winsock2.h"
#pragma comment(lib, "Ws2_32.lib") 
#include <iostream> 
#include <string> 
#include <thread>
#include <Windows.h>
#include <process.h>
#include <conio.h>
using namespace std;

#define BUFFER_SIZE 1024 * 256 // 256 килобайт
#define STATUS_OK 200

int isRunning;

int parseHeader(int sock) {
	char buff[BUFFER_SIZE], *ptr = buff + 4;
	int bytes_received, status;
	while (bytes_received = recv(sock, ptr, 1, 0)) {
		if (bytes_received == -1) { puts("error"); return -1; }
		if ((ptr[-3] == '\r') && (ptr[-2] == '\n') && (ptr[-1] == '\r') && (*ptr == '\n')) break;
		ptr++;
	}
	*ptr = 0;
	ptr = buff + 4;
	if (bytes_received) {
		ptr = strstr(ptr, "Content-Length:");
		if (ptr) sscanf(ptr, "%*s %d", &bytes_received);
		else bytes_received = 0;
	}
	return bytes_received;
}

int readHttpStatus(int sock) {
	char buff[BUFFER_SIZE], *ptr = buff + 1;
	int bytes_received, status;
	while (bytes_received = recv(sock, ptr, 1, 0)) {
		if (bytes_received == -1) { puts("errorS"); return -1; }
		if ((ptr[-1] == '\r') && (*ptr == '\n')) break;
		ptr++;
	}
	*ptr = 0;
	ptr = buff + 1;
	sscanf(ptr, "%*s %d ", &status);
	return (bytes_received > 0) ? status : 0;
}

int downloadImage(int sock, int length) {
	char buffer[BUFFER_SIZE];
	int bytes = 0, bytes_received;
	FILE* fd = fopen(string("test" + to_string(sock) + ".jpg").c_str(), "wb");
	while ((bytes_received = recv(sock, buffer, BUFFER_SIZE, 0))) {
		fwrite(buffer, 1, bytes_received, fd);
		bytes += bytes_received;
		if (bytes == length) break;
	}
	fclose(fd);
	closesocket(sock);
	return 0;
}

void readInputAndSend() {
	char buffer[BUFFER_SIZE];
	string url, domain_string, path_string;
	size_t pos;
	const char *domain, *path;
	int sock;
	struct hostent *he;
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(80);

	while (isRunning) {
		cout << "Enter image URL ('q' for exit): ";
		getline(cin, url);
		if (url == "q") isRunning = 0;
		else {
			pos = url.find('/');
			domain_string = url.substr(0, pos);
			path_string = url.substr(pos + 1);
			domain = domain_string.c_str();
			path = path_string.c_str();
			try {
				if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) throw string("socket");
				if ((he = gethostbyname(domain)) == NULL) throw string("gethostbyname");
				server_addr.sin_addr = *((struct in_addr *) he->h_addr);
				if (connect(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) throw string("connect");
				snprintf(buffer, sizeof(buffer), "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n", path, domain);
				if (send(sock, buffer, strlen(buffer), 0) == -1) throw string("send");
				if (readHttpStatus(sock) != STATUS_OK) throw string("readHttpStatus");
				int contentLengh = parseHeader(sock);
				cout << "contentLengh = " << contentLengh << endl;
				if (contentLengh) {
					thread t(downloadImage, sock, contentLengh);
					t.detach();
				}
			}

			catch (string error) {
				cout << "Error in '" << error << "' function: " << WSAGetLastError() << endl;
			}
		}
	}
}

int main() {
	char buffer[BUFFER_SIZE];

	if (WSAStartup(MAKEWORD(2, 2), (WSADATA *)&buffer[0])) {
		cout << "Error in 'WSAStartup' function: " << WSAGetLastError() << endl;
		return -1;
	}
	isRunning = 1;
	thread t(readInputAndSend);
	t.detach();
	while (isRunning) {
		Sleep(100);
	}
	WSACleanup();
	cout << "Press any key to exit...";
	getch();
	return 0;
}