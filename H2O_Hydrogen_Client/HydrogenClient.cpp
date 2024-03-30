#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ctime>
#include <fstream>
#include <string> // for std::string
#include <sstream> // for std::stringstream
#include <thread>
#include <mutex>
#include <condition_variable>


using namespace std;

std::mutex sendMutex;
std::condition_variable ackReceived;
bool isAckReceived = true;
mutex logMutex;
void sendRequest(SOCKET clientSocket, int startId, int endId, ofstream& logFile, mutex& logMutex) {
    const int maxChunkSize = 190; // Maximum size of each chunk
    int idLength = 0;
    if (endId <= 9) {
        idLength = 2; // IDs with 1 digit (0-9)
    }
    else if (endId <= 99) {
        idLength = 3; // IDs with 2 digits (10-99)
    }
    else if (endId <= 999) {
        idLength = 4; // IDs with 3 digits (100-999)
    }
    else if (endId <= 9999) {
        idLength = 5; // IDs with 4 digits (1000-9999)
    }
    else if (endId <= 99999) {
        idLength = 6; // IDs with 5 digits (10000-99999)
    }
    else {
        idLength = 7; // IDs with 6 digits (100000-999999)
    }

    int startIdStored = startId; // Initialize startIdStored
    int endIdStored = startId - 1; // Initialize endIdStored

    // Iterate over IDs and send in chunks
    for (int id = startId; id <= endId; ) {
        {
            unique_lock<mutex> lock(logMutex); // Use logMutex for synchronization
            ackReceived.wait(lock, [] { return isAckReceived; });
            isAckReceived = false; // Reset the flag after acknowledgment is received
        }
        string request;
        int remainingChars = maxChunkSize; // Remaining characters allowed in the current chunk

        // Build the request string with IDs until the chunk size limit is reached
        while (id <= endId && remainingChars > idLength) {
            string idStr = "H" + to_string(id) + " ";
            if (idStr.length() <= remainingChars) {
                request += idStr;
                remainingChars -= idStr.length();
                ++id;
            }
            else {
                break; // Stop if adding the ID would exceed the chunk size limit
            }
        }

        // Remove the trailing space from the request
        if (!request.empty()) {
            request.pop_back();
        }
        //cout << "Sent: " << "[" << request << "]" << endl;
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

        // Set up the file descriptor set for the socket
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(clientSocket, &readSet);

        // Set up the timeout
        timeval timeout;
        timeout.tv_sec = 5;  // 5 seconds
        timeout.tv_usec = 0;

        // Wait for incoming messages with a timeout of 5 seconds
        int result = select(0, &readSet, nullptr, nullptr, &timeout);
        if (result == SOCKET_ERROR) {
            cerr << "Error in select: " << WSAGetLastError() << endl;
            continue;
        }
        else if (result == 0) {
            cout << "No incoming message within 5 seconds. Continuing to listen." << endl;
            continue;
        }

        // Incoming message received, proceed to receive it
        byteCount = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (byteCount > 0) {
            buffer[byteCount] = '\0';

            // Check if "ack" or "bond" is found in the message
            string message(buffer);
            size_t foundAck = message.find("ack");
            size_t foundBond = message.find("bond");

            // Extract the IDs from the received message
            string extractedIDs;
            size_t foundColon = message.find(":");
            if (foundColon != string::npos) {
                extractedIDs = message.substr(foundColon + 2); // Skip ": "
            }

            //cout << "Received message: [" << extractedIDs << "]" << endl;

            // Record message in the corresponding log file with timestamp
            {
                lock_guard<mutex> lock(logMutex);
                time_t now = time(0);
                tm ltm;
                localtime_s(&ltm, &now);
                char timestamp[20];
                strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &ltm);
                if (foundAck != string::npos) {
                    // "ack" found in the message, log each ID to logFileAck individually
                    stringstream ss(extractedIDs);
                    string id;
                    while (getline(ss, id, ' ')) {
                        logFileAck << id << ", ack, " << timestamp << endl;
                    }
                }
                else if (foundBond != string::npos) {
                    // "bond" found in the message, log to logFileBond
                    logFileBond << extractedIDs << ", bonded, " << timestamp << endl;
                }
            }
        }
        {
            std::lock_guard<std::mutex> lock(sendMutex);
            isAckReceived = true;
            ackReceived.notify_all(); // Notify sendRequest to send the next batch
        }
    }
}



int main() {
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
    ofstream logFileReq("H_client_log_req.txt", ios::app);
    if (!logFileReq.is_open()) {
        cerr << "Failed to open log file." << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 0;
    }

    // Open log file for recording acknowledgements, truncating existing content
    ofstream logFileAck("H_client_log_ack.txt", ios::app);
    if (!logFileAck.is_open()) {
        cerr << "Failed to open log file." << endl;
        closesocket(clientSocket);
        WSACleanup();
        return 0;
    }

    // Open log file for recording bond messages, truncating existing content
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
    thread requestThread1(sendRequest, clientSocket, 1, 500, ref(logFileReq), ref(logFileReqMutex));

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
