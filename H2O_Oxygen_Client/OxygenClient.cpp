#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <sstream>
#include <chrono>
#include <ctime>
#include <thread>
using namespace std;



// Function definition for sendData
void sendData(SOCKET clientSocket, const std::string& message) {
    // Sending data
    int sbyteCount = send(clientSocket, message.c_str(), message.length(), 0);
    if (sbyteCount == SOCKET_ERROR) {
        cout << "Client send error: " << WSAGetLastError() << endl;
    }
    else {
        std::cout << message << std::endl;
        //    cout << "Client: sent " << sbyteCount << " bytes" << endl;
    }
}

void oxygenSend(SOCKET clientSocket, int oxygenInput) {

    for (int i = 1; i <= oxygenInput; ++i) {

        time_t now = time(0);
        struct tm* timeinfo;
        timeinfo = localtime(&now);

        char timestamp[20];
        strftime(timestamp, 20, "%Y-%m-%d %H:%M:%S", timeinfo);

        std::string message = "O" + std::to_string(i) + ", request, " + timestamp;


        // Send the message to the server
        sendData(clientSocket, message);


        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

}


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
    clientService.sin_addr.s_addr = inet_addr("192.168.254.186");
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

        int oxygenInput;
        std::cout << "Input number of Oxygen bond requests: ";
        std::cin >> oxygenInput;

        oxygenSend(clientSocket, oxygenInput);



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

    // do oxygen logic here 
    // take N, then send N times
    // receive confirmation

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}

