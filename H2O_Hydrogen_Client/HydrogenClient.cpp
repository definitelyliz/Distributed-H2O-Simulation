#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
using namespace std;

int main() {
    // Initialize WSA
    WSADATA wsaData;
    int wserr;
    WORD wVersionRequested = MAKEWORD(2, 2);
    wserr = WSAStartup(wVersionRequested, &wsaData);
    if (wserr != 0) {
        cout << "The winsock dll not found" << endl;
        return 0;
    }
    else {
        cout << "The Winsock dll found" << endl;
        cout << "The status: " << wsaData.szSystemStatus << endl;
    }

    // Create socket
    SOCKET clientSocket;
    clientSocket = INVALID_SOCKET;
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        cout << "Error at socket(): " << WSAGetLastError() << endl;
        WSACleanup();
        return 0;
    }
    else {
        cout << "socket is OK!" << endl;
    }

    // Connect to server
    sockaddr_in clientService;
    clientService.sin_family = AF_INET;
    // Update with server's IP address
    clientService.sin_addr.s_addr = inet_addr("192.168.1.2"); 
    clientService.sin_port = htons(55555);
    if (connect(clientSocket, (SOCKADDR*)&clientService, sizeof(clientService)) == SOCKET_ERROR) {
        cout << "Client: connect() - Failed to connect: " << WSAGetLastError() << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 0;
    }
    else {
        cout << "Client: Connect() is OK!" << endl;
        cout << "Client: Can start sending and receiving data..." << endl;
    }

    // sanity check if connecting correctly
    // while (true) {
    //     // Sending data
    //     char buffer[200];
    //     cout << "Enter the message: ";
    //     cin.getline(buffer, sizeof(buffer));
    //     int sbyteCount = send(clientSocket, buffer, strlen(buffer) + 1, 0);
    //     if (sbyteCount == SOCKET_ERROR) {
    //         cout << "Client send error: " << WSAGetLastError() << endl;
    //         break; // Exit the loop on send error
    //     }
    //     else {
    //         cout << "Client: sent " << sbyteCount << " bytes" << endl;
    //     }
    // }

    // do hydrogen logic here 
    // take N, then send N times
    // receive confirmation

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}