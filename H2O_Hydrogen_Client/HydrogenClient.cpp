#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ctime>
#include <fstream>
#include <thread>
#include <mutex>

using namespace std;

mutex logMutex;

void sendRequest(SOCKET clientSocket, int id, ofstream& logFile, mutex& logMutex) {
    char request[10];
    sprintf_s(request, sizeof(request), "H%d", id);

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
        logFile << request << ", request, "  << timestamp << endl;
    }
}

void listenMessages(SOCKET clientSocket, ofstream& logFileAck, ofstream& logFileBond, mutex& logMutex) {
    char buffer[200];
    int byteCount;

    // Listen for messages from the server
    while (true) {
        byteCount = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (byteCount > 0) {
            buffer[byteCount] = '\0';

            // Check if "ack" or "bond" is found in the message
            string message(buffer);
            size_t foundAck = message.find("ack");
            size_t foundBond = message.find("bond");

            // Extract the message (H1) from the buffer
            string extractedMessage;
            size_t foundColon = message.find(":");
            if (foundColon != string::npos) {
                extractedMessage = message.substr(foundColon + 2); // Skip ": "
            }

            cout << "Received message: " << extractedMessage << endl;

            // Record message in the corresponding log file with timestamp
            {
                lock_guard<mutex> lock(logMutex);
                time_t now = time(0);
                tm ltm;
                localtime_s(&ltm, &now);
                char timestamp[20];
                strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &ltm);
                if (foundAck != string::npos) {
                    // "ack" found in the message, log to logFileAck
                    logFileAck << extractedMessage  << ", acknowledged, "<< ", " << timestamp << endl;
                }
                else if (foundBond != string::npos) {
                    // "bond" found in the message, log to logFileBond
                    logFileBond << extractedMessage << ", bonded, " << ", " << timestamp << endl;
                }
            }

            break; // Break from loop after receiving the message
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
    ofstream logFileReq("H_client_log_req.txt", ios::app);
    if (!logFileReq.is_open()) {
        cerr << "Failed to open log file." << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 0;
    }
    // Open log file for recording requests and acknowledgements
    ofstream logFileAck("H_client_log_ack.txt", ios::app);
    if (!logFileAck.is_open()) {
        cerr << "Failed to open log file." << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 0;
    }

    ofstream logFileBond("H_client_log_bond.txt", ios::app);
    if (!logFileBond.is_open()) {
        cerr << "Failed to open log file." << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 0;
    }

    // Define mutexes for each log file
    mutex logFileReqMutex;
    mutex logFileAckMutex;
    mutex logFileBondMutex;

    // Simulate sending requests asynchronously
    //for now I'm dealing with 1 req, I will scale it later
    thread requestThread1(sendRequest, clientSocket, 1, ref(logFileReq), ref(logFileReqMutex));

    // Simulate listening for acknowledgements asynchronously
    thread listenThread1(listenMessages, clientSocket, ref(logFileAck), ref(logFileBond), ref(logFileAckMutex));



    // Join threads
    requestThread1.join();
    listenThread1.join();

    // Cleanup and close socket
    closesocket(clientSocket);
    WSACleanup();
    logFileReq.close();
    logFileAck.close();
    logFileBond.close();
    return 0;
}
