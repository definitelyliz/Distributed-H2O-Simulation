# Distributed-H2O-Simulation

Remember to configure the code for the IP Address. Use the IP Address of the machine running the Server.

## Visual Studio 2022 Configuration

**Include the Winsock Library:** Navigate to "Configuration Properties -> Linker -> Input" and add "ws2_32.lib" to the "Additional Dependencies".

**Disable Deprecated API Warnings:** Go to project properties, navigate to "Configuration Properties -> C/C++ -> Preprocessor", and add `_WINSOCK_DEPRECATED_NO_WARNINGS` to the "Preprocessor Definitions".
