#include <vector>
#include <thread>
#include <iostream>
#include <algorithm>
#include "server_logic.h"

int main() {
    const int port = 12345;

    // 1. Ініціалізація WinSock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed with error: " << result << std::endl;
        return 1;
    }

    // 2. Запуск сервера
    try {
        start_server(port);  // слухає і обробляє клієнтів
    }
    catch (const std::exception& ex) {
        std::cerr << "Server error: " << ex.what() << std::endl;
    }

    // 3. Завершення WinSock
    WSACleanup();

    return 0;
}
