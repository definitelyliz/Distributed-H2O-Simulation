//Author: Elijah Dayon and Elizabeth Celestino
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ctime>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <sstream>

using namespace std;
mutex logMutex;
mutex bondedIDsMutex;

void sendBondedIDs(SOCKET clientSocket, vector<int>* bondedIDs, const string& clientType, ofstream* logFileBond) {
    // Check if bondedIDs is empty
    if (bondedIDs->empty()) {
        return;
    }

    const int maxChunkSize = 190;
    int idLength = clientType.length() + to_string((*bondedIDs)[0]).length() + 1; 

    // Get current timestamp
    time_t now = time(0);
    tm ltm;
    localtime_s(&ltm, &now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &ltm);

    // Log bonded IDs if they exist
    for (int id : *bondedIDs) {
        *logFileBond << clientType << id << ", bonded, " << timestamp << endl;
    }

    // Prepare and send bonded IDs in chunks
    string bondedIDsStr = "bond:";
    size_t totalSize = 0;
    for (int id : *bondedIDs) {
        // Calculate the size of the current ID string
        size_t idSize = clientType.length() + to_string(id).length() + 1; 

        // Check if adding the next ID will exceed the maximum chunk size
        if (totalSize + idSize > maxChunkSize) {
            // Send the current content of bondedIDsStr
            cout << "(in) Sent bond: [" << bondedIDsStr.c_str() << "] Size: " << bondedIDsStr.length() << endl;
            send(clientSocket, bondedIDsStr.c_str(), bondedIDsStr.length(), 0);
            this_thread::sleep_for(chrono::milliseconds(200)); 
            bondedIDsStr = "bond:"; 
            totalSize = 0; 
        }
        bondedIDsStr += " " + clientType + to_string(id);
        totalSize += idSize; 
    }

    // Send the remaining content of bondedIDsStr
    if (!bondedIDsStr.empty()) {
        cout << "(out) Server sent bond: [" << bondedIDsStr.c_str() << "] Size: " << bondedIDsStr.length() << endl;
        send(clientSocket, bondedIDsStr.c_str(), bondedIDsStr.length(), 0);
        this_thread::sleep_for(chrono::milliseconds(200));
    }
}




void clientHandler(SOCKET clientSocket,
    ofstream& logFileReqH,
    ofstream& logFileReqO,
    ofstream& logFileBondH,
    ofstream& logFileBondO,
    vector<int>& HIDs,
    vector<int>& OIDs,
    vector<int>& bondedHIDs,
    vector<int>& bondedOIDs,
    mutex& HIDMutex,
    mutex& OIDMutex,
    mutex& logMutex) {
    char buffer[200];
    int byteCount;
    string clientType;
    vector<int> emptyBondedIDs; 

    vector<int>* bondedIDs = nullptr;
    ofstream* logFileBond = nullptr;

    while (true) {
        // Receive data from client
        byteCount = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (byteCount > 0) {
            buffer[byteCount] = '\0'; 

            string message(buffer);
            ofstream* logFile;
            vector<int>* IDs;
            mutex* idMutex;

            if (message.find("H") != string::npos) {
                clientType = "H"; 
                logFile = &logFileReqH;
                logFileBond = &logFileBondH;
                IDs = &HIDs;
                bondedIDs = &bondedHIDs;
                idMutex = &HIDMutex;
            }
            else {
                clientType = "O"; 
                logFile = &logFileReqO;
                logFileBond = &logFileBondO;
                IDs = &OIDs;
                bondedIDs = &bondedOIDs;
                idMutex = &OIDMutex;
            }

            stringstream ss(message);
            string token;
            vector<int> ids;

            // Extract all IDs from the message
            while (getline(ss, token, ' ')) {
                if (token.find(clientType) != string::npos) {
                    int id = stoi(token.substr(1)); 
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

            // Lock the mutex for the corresponding IDs
            lock_guard<mutex> idLock(*idMutex);
            for (int id : ids) {
                IDs->push_back(id); 
            }
            if (clientType == "H") {

                while (OIDs.size() == 0) {
                    cout << "Waiting for O_client"  << endl;
                    this_thread::sleep_for(chrono::milliseconds(200)); 
                }
            }
            else {
                while (HIDs.size() == 0) {

                    cout << "Waiting for H_client" <<  endl;
                    this_thread::sleep_for(chrono::milliseconds(200)); 
                }
            }

            // Send acknowledgment back to client with received IDs
            string ack = "ack:";
            for (int id : ids) {
                ack += " " + clientType + to_string(id); 
            }
            send(clientSocket, ack.c_str(), ack.length(), 0);

            lock_guard<mutex> bondedIDsLock(bondedIDsMutex);
            while (HIDs.size() >= 2 && OIDs.size() >= 1) {
                // Lock the mutex before accessing shared resources
                {
                    bondedHIDs.push_back(HIDs[0]);
                    bondedHIDs.push_back(HIDs[1]);
                    bondedOIDs.push_back(OIDs[0]);
                    HIDs.erase(HIDs.begin(), HIDs.begin() + 2);
                    OIDs.erase(OIDs.begin());
                }
            }

            // Check if there are bonded IDs to send
            if (!bondedIDs->empty()) {
                sendBondedIDs(clientSocket, bondedIDs, clientType, logFileBond);
            }

        }
        else if (!bondedIDs->empty()) {
            sendBondedIDs(clientSocket, bondedIDs, clientType, logFileBond);
        }
        bondedIDs->clear();

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
    ofstream logFileBondH("H_client_log_bond.txt", ios::trunc);
    if (!logFileBondH.is_open()) {
        cerr << "Failed to open log file." << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 0;
    }
    ofstream logFileBondO("O_client_log_bond.txt", ios::trunc);
    if (!logFileBondO.is_open()) {
        cerr << "Failed to open log file." << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 0;
    }

    vector<int> HIDs;
    vector<int> OIDs;
    vector<int> bondedHIDs;
    vector<int> bondedOIDs;
    mutex HIDMutex;
    mutex OIDMutex;
    mutex logMutex;

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
        threads.emplace_back(clientHandler, 
            clientSocket, 
            ref(logFileReqH), 
            ref(logFileReqO), 
            ref(logFileBondH),
            ref(logFileBondO),
            ref(HIDs), 
            ref(OIDs), 
            ref(bondedHIDs), 
            ref(bondedOIDs), 
            ref(HIDMutex), 
            ref(OIDMutex), 
            ref(logMutex));
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
    logFileBondH.close();
    logFileBondO.close();
    return 0;
}
