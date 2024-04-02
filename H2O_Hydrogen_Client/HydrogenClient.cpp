//Author: Elijah Dayon and Elizabeth Celestino
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ctime>
#include <fstream>
#include <string> 
#include <sstream> 
#include <thread>
#include <mutex>
#include <condition_variable>


using namespace std;

std::mutex sendMutex;
std::condition_variable ackReceived;
bool isAckReceived = true;
bool noResponse = false;
mutex logMutex;

void sendRequest(SOCKET clientSocket, int startId, int endId, ofstream& logFile, mutex& logMutex) {
    const int maxChunkSize = 190; 
    int idLength = 0;
    if (endId <= 9) {
        idLength = 2; 
    }
    else if (endId <= 99) {
        idLength = 3; 
    }
    else if (endId <= 999) {
        idLength = 4; 
    }
    else if (endId <= 9999) {
        idLength = 5; 
    }
    else if (endId <= 99999) {
        idLength = 6; 
    }
    else {
        idLength = 7; 
    }

    int startIdStored = startId; 
    int endIdStored = startId - 1; 

    // Iterate over IDs and send in chunks
    for (int id = startId; id <= endId; ) {
        {
            // logMutex for synchronization
            unique_lock<mutex> lock(logMutex);
            ackReceived.wait(lock, [] { return isAckReceived; });
            isAckReceived = false; 
        }
        if (noResponse) {
            break;
        }

        string request;
        int remainingChars = maxChunkSize; 

        while (id <= endId && remainingChars > idLength) {
            string idStr = "H" + to_string(id) + " ";
            if (idStr.length() <= remainingChars) {
                request += idStr;
                remainingChars -= idStr.length();
                ++id;
            }
            else {
                break; 
            }
        }

        if (!request.empty()) {
            request.pop_back();
        }
        cout << "Sent req: [" << request.c_str() << "] Size: " << request.length() << endl;

        // Send request to server
        send(clientSocket, request.c_str(), request.length(), 0);

        // Record request in log file with timestamp
        {
            lock_guard<mutex> lock(logMutex);
            time_t now = time(0);
            tm ltm;
            localtime_s(&ltm, &now);
            char timestamp[20];
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &ltm);
            for (int i = startIdStored; i < id; ++i) { // Iterate from startIdStored to id
                string idStr = "H" + to_string(i);
                logFile << idStr << ", request, " << timestamp << endl;
            }
            startIdStored = id; // Update startIdStored
            endIdStored = id - 1; // Update endIdStored
        }
    }
}


void listenMessages(SOCKET clientSocket, ofstream& logFileAck, ofstream& logFileBond, mutex& logMutex) {
    while (true) {
        char buffer[200];
        int byteCount;

        // file descriptor set for the socket
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(clientSocket, &readSet);

        timeval timeout;
        timeout.tv_sec = 7;
        timeout.tv_usec = 0;

        // Wait for incoming messages
        int result = select(0, &readSet, nullptr, nullptr, &timeout);
        if (result == SOCKET_ERROR) {
            cerr << "Error in select: " << WSAGetLastError() << endl;
            continue;
        }
        else if (result == 0) {
            cout << "No incoming message within 20 seconds. Stopped listening." << endl;
            noResponse = true;
            break;
        }

        // Incoming message received, proceed to receive it
        byteCount = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (byteCount <= 0)
            break;

        if (byteCount > 0) {
            buffer[byteCount] = '\0';
            string message(buffer);
            size_t foundColon = message.find(":");
            if (foundColon != string::npos) {
                string messageType = message.substr(0, foundColon); 
                string extractedIDs = message.substr(foundColon + 2);

                {
                    lock_guard<mutex> lock(logMutex);
                    time_t now = time(0);
                    tm ltm;
                    localtime_s(&ltm, &now);
                    char timestamp[20];
                    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &ltm);

                    // Log the message based on its type
                    if (messageType == "ack") {
                        stringstream ss(extractedIDs);
                        string id;
                        while (getline(ss, id, ' ')) {
                            logFileAck << id << ", ack, " << timestamp << endl;
                        }
                        {
                            std::lock_guard<std::mutex> lock(sendMutex);
                            isAckReceived = true;
                            ackReceived.notify_all(); 
                        }
                    }
                    else if (messageType == "bond") {
                        stringstream ss(extractedIDs);
                        string id;
                        while (getline(ss, id, ' ')) {
                            logFileBond << id << ", bonded, " << timestamp << endl;
                        }
                    }
                    else {
                        cerr << "Invalid message type received: " << messageType << endl;
                        cerr << "Received invalid message: " << message << endl;
                    }
                }
            }
            else {
                cerr << "Invalid message format: " << message << endl;
                cerr << "Received invalid message: " << message << endl;
            }
        }
    }
}

int main() {
    int number;
    cout << "Hydrogen Client" << endl;
    cout << "=========================" << endl;
    // Initialize WSA variables
    WSADATA wsaData;
    int wsaerr;
    WORD wVersionRequested = MAKEWORD(2, 2);
    wsaerr = WSAStartup(wVersionRequested, &wsaData);
    if (wsaerr != 0) {
        cout << "The Winsock dll not found" << endl;
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
        cout << "Socket is on" << endl;
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

    // Open log file for recording requests and acknowledgements, truncating existing content
    ofstream logFileReq("H_client_log_req.txt", ios::trunc);
    if (!logFileReq.is_open()) {
        cerr << "Failed to open log file." << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 0;
    }

    ofstream logFileAck("H_client_log_ack.txt", ios::trunc);
    if (!logFileAck.is_open()) {
        cerr << "Failed to open log file." << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 0;
    }

    ofstream logFileBond("H_client_log_bond.txt", ios::trunc);
    if (!logFileBond.is_open()) {
        cerr << "Failed to open log file." << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 0;
    }

    mutex logFileReqMutex;
    mutex logFileAckMutex;
    mutex logFileBondMutex;

    std::cout << "Please enter M: ";
    std::cin >> number;

    // sending requests asynchronously
    thread requestThread1(sendRequest, clientSocket, 1, number, ref(logFileReq), ref(logFileReqMutex));

    // listening for acknowledgements asynchronously
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
