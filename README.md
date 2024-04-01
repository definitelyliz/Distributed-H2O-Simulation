# Project Name

Distributed-H2O-Simulation

## Introduction

C++ app demonstrating client-Server computing. There are 2 separate clients communicating with server concurrently. The goal of this activity is to explore the usage of request/sent/ack signals and comparing the results in clients and server.
The sanity check is easily done with text files.

## How to run the application

1. Clone this repository to your local machine

2. From file explorer, navigate to the project directories open the following extensions. You would have 3 separate VS2022 IDEs.

   ```bash
   Distributed_H2O_Simulation
   |-- H20_Oxygen_Client
   |   |-- H2O_Oxygen_Client.vcxproj
   |-- H20_Hydrogen_Client
   |   |-- H2O_Hydrogen_Client.vcxproj
   |-- H20_Server
   |   |-- H2O_Server.vcxproj
   ```

3. Configuring VS2022

   **Include the Winsock Library:** Click Solutions Explorer, select Project Folder (e.g. H2O_Hydrogen_Client) -> "Configuration Properties -> Linker -> Input" and add "ws2_32.lib" to the "Additional Dependencies". And do this for H2O_Oxygen_Client and H2O_Server folder.

   **Disable Deprecated API Warnings:** Go to project properties, select Project Folder (e.g. H2O_Hydrogen_Client) "Configuration Properties -> C/C++ -> Preprocessor", and add `_WINSOCK_DEPRECATED_NO_WARNINGS` to the "Preprocessor Definitions". And do this for other project folders.

4. Drag the following files from file explorer and drop it to the Source Files.

   H20_Hydrogen_Client:

   ```bash
      |-- Source Files
      |   |-- H_client_log_ack.txt
      |   |-- H_client_log_bond.txt
      |   |-- H_client_log_req.txt
      |   |-- HydrogenClient.cpp
   ```

   H20_Oxygen_Client:

   ```bash
      |-- Source Files
      |   |-- O_client_log_ack.txt
      |   |-- O_client_log_bond.txt
      |   |-- O_client_log_req.txt
      |   |-- OxygenClient.cpp
   ```

   H20_Server:

   ```bash
      |-- Source Files
      |   |-- H_client_log_req.txt
      |   |-- H_client_log_bond.txt
      |   |-- O_client_log_req.txt
      |   |-- O_client_log_bond.txt
      |   |-- Server.cpp
   ```

5. To configure the IP address of server, first navigate to HydrogenClient.cpp and OxygenClient.cpp and manually edit the inet_addr("Host's IPv4 Address") to actual server's IPv4 address.

6. Run the application in order, Server first followed by 2 clients.

7. Enter M number to both clients which represents number of requests that will be sent to the server.

8. Wait until one of the programs terminates. Stop all the programs and perform sanity check by comparing the results of clients and server.
