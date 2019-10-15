// OSSelect.cpp : Defines the entry point for the console application.
//

#include "winsock2.h"
#pragma comment(lib, "Ws2_32.lib") 
#include <iostream> 
#include <string> 
#include <thread>
#include <Windows.h>
#include <process.h>
#include <conio.h>
#include <map>

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
	int bytes_received, status;
	while (bytes_received = recv(sock, ptr, 1, 0)) {
		if (bytes_received == -1) return 0;
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
fd_set master_set, working_set;
map<int, int> master_map, working_map;
int isWorking = 1;

//  master_map.insert(pair<int, int>(451, 102040));
//  auto it = master_map.begin();
//  while (it != master_map.end()) {
//  	if (it == prev(master_map.end())) {
//  		cout << "last" << endl;
//  	}
//  	master_map.erase(451);
//  	it = master_map.begin();
//  	cout << "hello" << endl;
//  }

void manageSockets(void* data) {
	int c;
	struct timeval timeout;
	timeout.tv_sec = 60;
	timeout.tv_usec = 0;
	while (isWorking) {
		Sleep(500);
		memcpy(&working_set, &master_set, sizeof(master_set));
		memcpy(&working_map, &master_map, sizeof(master_map));
		c = select(0, &working_set, NULL, NULL, &timeout);
		if (c < 1) continue;
		for (auto it = working_map.begin(); it != working_map.end(); ) {
			int socket = (*it).first, contentLengh = (*it).second;
			cout << "start    " << socket << "    " << contentLengh << endl;
			if (FD_ISSET(socket, &working_set)) {
				cout << "socket    " << socket << "  ISSET  " << endl;
				int bytes = 0, bytes_received;
				char buffer[BUFFER_SIZE];
				FILE* fd = fopen(("image" + to_string(socket) + ".jpg").c_str(), "ab");
				while (true) {
					bytes_received = recv(socket, buffer, BUFFER_SIZE, 0);
					cout << socket << " recv1 " << bytes_received << endl;
					if (bytes_received < 0) {
						cout << socket << " stop " << endl;
						break;
					}
					fwrite(buffer, 1, bytes_received, fd);
					bytes += bytes_received;
					cout << socket << " recv2 " << bytes_received << "  B =  " << bytes << "  L =  " << contentLengh << endl;
					if (bytes == contentLengh) break;
				}



				//working_map.insert(pair<int, int>(11, 1024));
				//working_map.insert(pair<int, int>(22, 2048));
				//working_map.insert(pair<int, int>(33, 4096));
				//for (auto it = working_map.begin(); it != working_map.end(); ) {
				//	int socket = (*it).first, contentLengh = (*it).second;
				//
				//	if (it == working_map.begin()) {
				//		cout << "start" << endl;
				//		it++;
				//		working_map.erase(11);
				//	}
				//	else {
				//		it++;
				//	}
				//
				//	cout << socket << " " << contentLengh << endl;
				//}





				fclose(fd);
				contentLengh -= bytes;
				cout << "3    end " << contentLengh << endl;
				if (contentLengh == 0) {
					cout << "4.1 contentLengh == 0 " << endl;
					closesocket(socket);
					cout << "4.1 closesocket " << endl;
					FD_CLR(socket, &master_set);
					cout << "4.1 FD_CLR " << endl;
					it++;
					cout << "4.1 it++ done " << endl;
					master_map.erase(socket);
					cout << "4.1 master_map erase done " << endl;
					working_map.erase(socket);
					cout << "4.1 working_map erase done (all done)" << endl;
				}
				else {
					master_map.at(socket) = contentLengh;
					cout << "4.2 change done " << endl;
					++it;
					cout << "4.2 ++it done " << endl;
				}
			}
		}
	}
	_endthread();
}

void downloadImage(const char* domain, const char* path) {
	int sock;
	BOOL on = TRUE;
	struct hostent* he;
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(80);
	char buffer[BUFFER_SIZE];
	try {
		if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) throw string("socket");
		if ((he = gethostbyname(domain)) == NULL) throw string("gethostbyname");
		server_addr.sin_addr = *((struct in_addr*) he->h_addr);
		if (connect(sock, (struct sockaddr*) & server_addr, sizeof(struct sockaddr)) == -1) throw string("connect");
		snprintf(buffer, sizeof(buffer), "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n", path, domain);
		if (send(sock, buffer, strlen(buffer), 0) == -1) throw string("send");
		if (getStatus(sock) != STATUS_OK) throw string("readHttpStatus");
		int contentLengh = getContentLength(sock);
		if (contentLengh == 0) throw string("getContentLength");
		ioctlsocket(sock, FIONBIO, (unsigned long*)& on);
		FD_SET(sock, &master_set);
		master_map.insert(pair<int, int>(sock, contentLengh));
	}
	catch (string error) {
		cout << "Error in '" << error << "' function: " << WSAGetLastError() << endl;
	}
}

int main()
{
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata)) {
		cout << "Error in 'WSAStartup' function: " << WSAGetLastError() << endl;
		return -1;
	}

	string url, domain, path;

	FD_ZERO(&master_set);
	_beginthread(&manageSockets, 0, 0);

	while (true) {
		cout << "Enter image URL ('q' for exit): ";
		getline(cin, url);
		if (url == "q") break;
		else if (url == "t") {
			downloadImage("localhost", "image.jpg");
			downloadImage("localhost", "image.jpg");
			downloadImage("localhost", "image.jpg");
			downloadImage("localhost", "image.jpg");
			downloadImage("localhost", "image.jpg");
		}
		else {
			getPathAndDomain(&url, &domain, &path);
			downloadImage(domain.c_str(), path.c_str());
		}
	}

	isWorking = 0;
	cout << "Press any key to exit...";
	_getch();
	WSACleanup();
	return 0;
}