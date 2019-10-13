// OS Lab 3.cpp : Defines the entry point for the console application.
//
#include "winsock2.h"
#pragma comment(lib, "Ws2_32.lib") 
#include <iostream> 
#include <string> 
#include <thread>
#include <Windows.h>
#include <process.h>
#include <conio.h>

using namespace std;

#define BUFFER_SIZE 1024 * 256
#define STATUS_OK 200
#define FILTER_LENGTH 3

void getPathAndDomain(string* inputString, string* domain, string* path) {
	string filter[FILTER_LENGTH] = { "https://", "http://", "www." };
	size_t pos;
	for (int i = 0; i < FILTER_LENGTH; i++) {
		pos = (*inputString).find(filter[i]);
		if (pos != string::npos) (*inputString).erase(pos, filter[i].length());
	}
	pos = (*inputString).find('/');
	*domain = (*inputString).substr(0, pos);
	*path = (*inputString).substr(pos + 1);
}

int getContentLength(int sock) {
	char buff[BUFFER_SIZE], * ptr = buff + 4;
	int bytes_received;
	while (bytes_received = recv(sock, ptr, 1, 0)) {
		if (bytes_received == -1) return -1;
		if ((ptr[-3] == '\r') && (ptr[-2] == '\n') && (ptr[-1] == '\r') && (*ptr == '\n')) break;
		ptr++;
	}
	*ptr = 0;
	ptr = buff + 4;
	if (bytes_received) {
		ptr = strstr(ptr, "Content-Length:");
		if (ptr) sscanf(ptr, "%*s%d", &bytes_received);
		else bytes_received = 0;
	}
	return bytes_received;
}

int getStatus(int sock) {
	char buff[BUFFER_SIZE], * ptr = buff + 1;
	int bytes_received, status;
	while (bytes_received = recv(sock, ptr, 1, 0)) {
		if (bytes_received == -1) return -1;
		if ((ptr[-1] == '\r') && (*ptr == '\n')) break;
		ptr++;
	}
	*ptr = 0;
	ptr = buff + 1;
	sscanf(ptr, "%*s%d", &status);
	return (bytes_received > 0) ? status : 0;
}

void donwloadImage(void* socket) {
	int sock = (int)socket;
	int contentLengh = getContentLength(sock);
	int bytes = 0, bytes_received;
	char buffer[BUFFER_SIZE];
	if (contentLengh) {
		FILE* fd = fopen(("test" + to_string(sock) + "_" + to_string((rand() % 1000)) + ".jpg").c_str(), "wb");
		while ((bytes_received = recv(sock, buffer, BUFFER_SIZE, 0))) {
			fwrite(buffer, 1, bytes_received, fd);
			bytes += bytes_received;
			if (bytes == contentLengh) break;
		}
		fclose(fd);
		closesocket(sock);
	}
	return;
}

int main() {

	// "https://cdn.hasselblad.com/samples/x1d-II-50c/x1d-II-sample-01.jpg"
	// "https://www.lifeofpix.com/wp-content/uploads/2019/10/DSC00873-1600x2396.jpg"
	// "https://images.unsplash.com/photo-1565028832942-d005b1bd84f0?ixlib=rb-1.2.1&ixid=eyJhcHBfaWQiOjEyMDd9&auto=format&fit=crop&w=1950&q=80"
	// "https://images.pexels.com/photos/1955134/pexels-photo-1955134.jpeg?cs=srgb&dl=adventure-asphalt-daylight-1955134.jpg&fm=jpg"
	// "https://image.freepik.com/free-photo/tropical-foliage-background-with-blank-card_24972-451.jpg"
	// "https://cdn.pixabay.com/photo/2019/09/30/18/41/taxi-4516525_960_720.jpg"
	// "http://www.effigis.com/wp-content/uploads/2015/02/Airbus_Pleiades_50cm_8bit_RGB_Yogyakarta.jpg"

	char buffer[BUFFER_SIZE];

	string url, domain, path;

	int sock;

	struct hostent* he;

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(80);

	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata)) {
		cout << "Error in 'WSAStartup' function: " << WSAGetLastError() << endl;
		return -1;
	}
	while (true) {
		cout << "Enter image URL ('q' for exit): ";
		getline(cin, url);
		if (url == "q") break;

		getPathAndDomain(&url, &domain, &path);

		try {
			if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) throw string("socket");

			if ((he = gethostbyname(domain.c_str())) == NULL) throw string("gethostbyname");

			server_addr.sin_addr = *((struct in_addr*) he->h_addr);

			if (connect(sock, (struct sockaddr*) & server_addr, sizeof(struct sockaddr)) == -1) throw string("connect");

			snprintf(buffer, sizeof(buffer), "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n", path.c_str(), domain.c_str());
			if (send(sock, buffer, strlen(buffer), 0) == -1) throw string("send");

			if (getStatus(sock) != STATUS_OK) throw string("readHttpStatus");

			_beginthread(&donwloadImage, 0, (void*)sock);
		}
		catch (string error) {
			cout << "Error in '" << error << "' function: " << WSAGetLastError() << endl;
		}
	}
	cout << "Press any key to exit...";
	_getch();
	return 0;
}