#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ctime>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <sstream> // for std::stringstream

using namespace std;

mutex logMutex;

void clientHandler(SOCKET clientSocket, ofstream& logFileReqH, ofstream& logFileReqO) {
    char buffer[200];
    int byteCount;

    while (true) {
        // Receive data from client
        byteCount = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (byteCount > 0) {
            buffer[byteCount] = '\0'; // Null-terminate the received data

            // Detect client type ('H' or 'O')
            string clientType;
            string message(buffer);
            ofstream* logFile;
            if (message.find("H") != string::npos) {
                clientType = "H"; // Set client type to 'H' if "H" is detected
                logFile = &logFileReqH;
            }
            else {
                clientType = "O"; // Otherwise, set client type to 'O'
                logFile = &logFileReqO;
            }
            stringstream ss(message);
            string token;
            vector<int> ids;

            // Extract all IDs from the message
            while (getline(ss, token, ' ')) {
                if (token.find(clientType) != string::npos) {
                    int id = stoi(token.substr(1)); // Extract the ID after "H"
                    ids.push_back(id);
                }
            }

            // Record request IDs in log file with timestamp
            {
                lock_guard<mutex> lock(logMutex);
                time_t now = time(0);
                tm ltm;
                localtime_s(&ltm, &now);
                char timestamp[20];
                strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &ltm);
                for (int id : ids) {
                    *logFile << clientType << id << ", received, " << timestamp << endl;
                }
            }

            // Send acknowledgment back to client with received IDs
            string ack = "ack:";
            for (int id : ids) {
                ack += " " + clientType + to_string(id); // Append each ID to the acknowledgment
            }
            cout << "[" << ack << "]" << endl;
            send(clientSocket, ack.c_str(), ack.length(), 0);
        }
    }
}

int main() {
    cout << "Water Server" << endl;
    cout << "=========================" << endl;
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
    ofstream logFileReqH("H_client_log_req.txt", ios::trunc);
    if (!logFileReqH.is_open()) {
        cerr << "Failed to open log file." << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 0;
    }
    ofstream logFileReqO("O_client_log_req.txt", ios::trunc);
    if (!logFileReqO.is_open()) {
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
            cout << "accept() is on" << endl;
        }

        // Start a new thread to handle the client
        threads.emplace_back(clientHandler, clientSocket, ref(logFileReqH), ref(logFileReqO));
    }

    // Join all threads
    for (auto& th : threads) {
        th.join();
    }

    // Cleanup and close sockets
    closesocket(serverSocket);
    WSACleanup();
    logFileReqH.close();
    logFileReqO.close();
    return 0;
}
