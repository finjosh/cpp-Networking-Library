# Networking-Library
A simple and efficient C++ networking library built using ([SFML](https://www.sfml-dev.org/index.php))'s networking module.

### Tested with: 
  - Compiler: g++
  - Version: g++.exe (Rev2, Built by MSYS2 project) 14.2.0
  - To use the prebuilt libs you have to use the ucrt MSYS2 mingw build for g++ 14.2.0

### [SFML](https://www.sfml-dev.org/index.php)
    - Version: 3.0.0

### [TGUI](https://tgui.eu/)
    - TGUI is used for the Socket UI and Connection Display, which are not required for the networking library to work
    - Version: 1.7

### [cpp-Utilities](https://github.com/finjosh/cpp-Utilities)
    - Built with the latest release

# Class breakdown
| File | Brief Description | Dependencies |
| --- | --- | --- |
| `Socket.hpp` | Stores data that is useful for a server or client. Derived from the SFML UDP socket. Can be derived from to create your own implementation of a client and server | SFML Networking and time, cpp-Utilities(funcHelper.hpp, EventHelper.hpp, and UpdateLimiter.hpp) |
| `Client.hpp` | An implementation of a client | Socket.hpp |
| `ClientData.hpp` | Client data that is stored in the server | SFML data types |
| `Server.hpp` | An implementation of a server | Socket.hpp and ClientData.hpp |
| `SocketUI.hpp` | UI system for connecting/hosting and displaying the connection information (only works with built-in client and server). Changing the default TGUI theme, the SocketUI will also update | TGUI, Client.hpp, Server.hpp, cpp-Utilities(TerminatingFunction.hpp) |

# Socket UI

<div align="center">
  <p>
    <img src=https://github.com/finjosh/Networking-Library/assets/109707607/04bb0551-d1c6-4efa-b4b5-9c357e53afb3>
    <img src=https://github.com/finjosh/Networking-Library/assets/109707607/80183637-c832-4729-aa9c-69474b31da2c>
    <p>
    <img src=https://github.com/finjosh/Networking-Library/assets/109707607/4b9ab384-ec4a-4124-9741-0288fe2e7f7d>
    <img src=https://github.com/finjosh/Networking-Library/assets/109707607/11b3f9dc-877a-4711-942d-54121296cccc>
  </p>
</div>

