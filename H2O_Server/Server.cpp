#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ctime>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>

using namespace std;

mutex logMutex;

void clientHandler(SOCKET clientSocket, ofstream& logFile) {
    char buffer[200];
    int byteCount;

    while (true) {
        // Receive data from client
        byteCount = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (byteCount > 0) {
            buffer[byteCount] = '\0'; // Null-terminate the received data
            cout << buffer << endl;
            // Record request in log file with timestamp
            {
                lock_guard<mutex> lock(logMutex);
                time_t now = time(0);
                tm ltm;
                localtime_s(&ltm, &now);
                char timestamp[20];
                strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &ltm);
                logFile << "Received request: " << buffer << " at " << timestamp << endl;
            }

            // Send acknowledgement back to client
            string ack = "ack: ";
            ack += buffer; // Acknowledge the received request
            send(clientSocket, ack.c_str(), ack.length(), 0);
            cout << "Ack sent" << endl;
        }
    }
}

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
        cout << "Socket online" << endl;
    }

    // Bind the socket
    sockaddr_in service;
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = INADDR_ANY; // Bind to all available interfaces
    service.sin_port = htons(55555);
    if (bind(serverSocket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
        cout << "bind() failed: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 0;
    }
    else {
        cout << "bind() on" << endl;
    }

    // Listen to incoming connections
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        cout << "listen(): Error listening on socket: " << WSAGetLastError() << endl;
    }
    else {
        cout << "listen() is online, waiting for new connections..." << endl;
    }

    // Open log file for recording requests
    ofstream logFile("server_log.txt", ios::app);
    if (!logFile.is_open()) {
        cerr << "Failed to open log file." << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 0;
    }

    vector<thread> threads;

    // Accept incoming connections and handle clients concurrently
    while (true) {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            cout << "accept failed: " << WSAGetLastError() << endl;
            closesocket(serverSocket);
            WSACleanup();
            return -1;
        }
        else {
            cout << "accept() is OK!" << endl;
        }

        // Start a new thread to handle the client
        threads.emplace_back(clientHandler, clientSocket, ref(logFile));
    }

    // Join all threads
    for (auto& th : threads) {
        th.join();
    }

    // Cleanup and close sockets
    closesocket(serverSocket);
    WSACleanup();
    logFile.close();
    return 0;
}
