# pragma once

#include"include/mnetwork.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <thread>
#include <atomic>
#include <memory>
#include <functional>
#include <regex>
#include <chrono>
#include <mutex>
#include <iomanip>
#include <ctime>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define close closesocket
    #define SHUT_RDWR SD_BOTH
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <sys/select.h>
#endif
