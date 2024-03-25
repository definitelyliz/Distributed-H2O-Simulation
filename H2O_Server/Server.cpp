#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ctime>
using namespace std;

int main() {
    // Initialize WSA variables
    WSADATA wsaData;
    int wsaerr;
    WORD wVersionRequested = MAKEWORD(2, 2);
    wsaerr = WSAStartup(wVersionRequested, &wsaData);
    if (wsaerr != 0) {
        cout << "The Winsock dll not found!" << endl;
        return 0;
    }
    else {
        cout << "The Winsock dll found" << endl;
        cout << "The status: " << wsaData.szSystemStatus << endl;
    }

    // Create socket
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        cout << "Error at socket(): " << WSAGetLastError() << endl;
        WSACleanup();
        return 0;
    }
    else {
        cout << "Socket is OK!" << endl;
    }

    // Bind the socket
    sockaddr_in service;
    service.sin_family = AF_INET;
    // Change to server machine ip address
    service.sin_addr.s_addr = inet_addr("192.168.1.2");
    service.sin_port = htons(55555);
    if (bind(serverSocket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
        cout << "bind() failed: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 0;
    }
    else {
        cout << "bind() is OK!" << endl;
    }

    // Listen to incoming connections
    if (listen(serverSocket, 2) == SOCKET_ERROR) { // Increase backlog to 2 for both clients
        cout << "listen(): Error listening on socket: " << WSAGetLastError() << endl;
    }
    else {
        cout << "listen() is OK!, I'm waiting for new connections..." << endl;
    }

    // Accept incoming connections from hydrogen and oxygen clients
    SOCKET hydrogenClientSocket = accept(serverSocket, NULL, NULL);
    SOCKET oxygenClientSocket = accept(serverSocket, NULL, NULL);

    if (hydrogenClientSocket == INVALID_SOCKET || oxygenClientSocket == INVALID_SOCKET) {
        cout << "accept failed: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }
    else {
        cout << "accept() is OK!" << endl;
    }

    // sanity check if connecting correctly
    // while (true) {
    //     char buffer[200];
    //     int byteCount;

    //     // Receive data from hydrogen client
    //     byteCount = recv(hydrogenClientSocket, buffer, sizeof(buffer), 0);
    //     if (byteCount > 0) {
    //         buffer[byteCount] = '\0'; // Null-terminate the received data
    //         cout << "Received from hydrogen client: " << buffer << endl;
    //     }

    //     // Receive data from oxygen client
    //     byteCount = recv(oxygenClientSocket, buffer, sizeof(buffer), 0);
    //     if (byteCount > 0) {
    //         buffer[byteCount] = '\0'; // Null-terminate the received data
    //         cout << "Received from oxygen client: " << buffer << endl;
    //     }
    // }

    // Handle communication with clients here...
    // Receive and send data accordingly (receive requests and send confirmation here)


    // Cleanup and close sockets
    closesocket(hydrogenClientSocket);
    closesocket(oxygenClientSocket);
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
