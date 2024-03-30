#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ctime>
#include <fstream>
#include <thread>
#include <mutex>

using namespace std;

mutex logMutex;

void sendRequest(SOCKET clientSocket, int id, ofstream& logFile) {
    char request[10];
    sprintf_s(request, sizeof(request), "O%d", id);

    // Send request to server
    send(clientSocket, request, strlen(request), 0);

    // Record request in log file with timestamp
    {
        lock_guard<mutex> lock(logMutex);
        time_t now = time(0);
        tm ltm;
        localtime_s(&ltm, &now);
        char timestamp[20];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &ltm);
        logFile << request << ", request, " << timestamp << endl;
    }
}

void listenAck(SOCKET clientSocket, int id, ofstream& logFile) {
    char buffer[200];
    int byteCount;

    // Listen for acknowledgements from server
    while (true) {
        byteCount = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (byteCount > 0) {
            buffer[byteCount] = '\0';

            if (strstr(buffer, "ack")) {

                // Record acknowledgement in log file with timestamp
                {
                    lock_guard<mutex> lock(logMutex);
                    time_t now = time(0);
                    tm ltm;
                    localtime_s(&ltm, &now);
                    char timestamp[20];
                    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &ltm);
                    logFile << id << ", acknowledged, " << timestamp << endl;
                }

                break; // Break from loop after receiving acknowledgement
            }
        }
    }
}

void listenBond(SOCKET clientSocket, int id, ofstream& logFile) {
    char buffer[200];
    int byteCount;

    // Listen for bonded from server
    while (true) {
        byteCount = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (byteCount > 0) {
            buffer[byteCount] = '\0';
            cout << buffer << endl;
            // Record acknowledgement in log file with timestamp
            if (strstr(buffer, "bond")) {

                {
                    lock_guard<mutex> lock(logMutex);
                    time_t now = time(0);
                    tm ltm;
                    localtime_s(&ltm, &now);
                    char timestamp[20];
                    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &ltm);
                    logFile << id << ", ack, " << timestamp << endl;
                }

                break; // Break from loop after receiving acknowledgement
            }
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
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        cout << "Error at socket(): " << WSAGetLastError() << endl;
        WSACleanup();
        return 0;
    }
    else {
        cout << "Socket is OK!" << endl;
    }

    // Connect to server
    sockaddr_in service;
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = inet_addr("192.168.68.105"); // Change to server IP address
    service.sin_port = htons(55555); // Change to server port
    if (connect(clientSocket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
        cout << "Failed to connect." << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 0;
    }
    else {
        cout << "Connected to server." << endl;
    }

    // Open log file for recording requests and acknowledgements
    ofstream logFileReq("O_client_log_req.txt", ios::app);
    if (!logFileReq.is_open()) {
        cerr << "Failed to open log file." << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 0;
    }
    // Open log file for recording requests and acknowledgements
    ofstream logFileAck("O_client_log_ack.txt", ios::app);
    if (!logFileAck.is_open()) {
        cerr << "Failed to open log file." << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 0;
    }

    ofstream logFileBond("O_client_log_bond.txt", ios::app);
    if (!logFileBond.is_open()) {
        cerr << "Failed to open log file." << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 0;
    }

    // Simulate sending requests asynchronously
    thread requestThread1(sendRequest, clientSocket, 1, ref(logFileReq));

    // Simulate listening for acknowledgements asynchronously
    thread ackThread1(listenAck, clientSocket, 1, ref(logFileAck));

    // Simulate listening for acknowledgements asynchronously
    thread bondThread1(listenBond, clientSocket, 1, ref(logFileBond));


    // Join threads
    requestThread1.join();
    ackThread1.join();

    // Cleanup and close socket
    closesocket(clientSocket);
    WSACleanup();
    logFileReq.close();
    logFileAck.close();
    return 0;
}
