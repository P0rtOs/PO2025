#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <vector>
#include <winsock2.h>
#include <cstdint>
#include <cstring>

#pragma comment(lib, "ws2_32.lib")

bool send_all(SOCKET sock, const char* data, int length) {
    int total_sent = 0;
    while (total_sent < length) {
        int sent = send(sock, data + total_sent, length - total_sent, 0);
        if (sent <= 0) return false;
        total_sent += sent;
    }
    return true;
}

bool recv_all(SOCKET sock, char* data, int length) {
    int total_received = 0;
    while (total_received < length) {
        int received = recv(sock, data + total_received, length - total_received, 0);
        if (received <= 0) return false;
        total_received += received;
    }
    return true;
}

bool send_tlv(SOCKET sock, uint8_t type, const std::vector<char>& value) {
    uint32_t length = static_cast<uint32_t>(value.size());
    char header[5];
    header[0] = type;
    header[1] = (length >> 24) & 0xFF;
    header[2] = (length >> 16) & 0xFF;
    header[3] = (length >> 8) & 0xFF;
    header[4] = length & 0xFF;

    return send_all(sock, header, 5) && send_all(sock, value.data(), length);
}

bool receive_result(SOCKET sock, std::vector<int32_t>& matrix, uint32_t& rows, uint32_t& cols) {
    char header[5];
    if (!recv_all(sock, header, 5)) return false;

    uint8_t type = static_cast<uint8_t>(header[0]);

    uint32_t length = (static_cast<uint8_t>(header[1]) << 24) |
        (static_cast<uint8_t>(header[2]) << 16) |
        (static_cast<uint8_t>(header[3]) << 8) |
        (static_cast<uint8_t>(header[4]));

    std::vector<char> value(length);
    if (!recv_all(sock, value.data(), length)) return false;

    if (type == 0xFF) {
        std::string error_msg(value.begin(), value.end());
        std::cerr << "[Client][Error] " << error_msg << std::endl;
        return false;
    }

    if (type != 0x04) {
        std::cerr << "[Client] Unexpected TLV type: 0x" << std::hex << int(type) << std::endl;
        return false;
    }

    std::memcpy(&rows, value.data(), 4);
    std::memcpy(&cols, value.data() + 4, 4);

    rows = ntohl(rows);
    cols = ntohl(cols);

    matrix.resize(rows * cols);
    std::memcpy(matrix.data(), value.data() + 8, rows * cols * sizeof(int32_t));
    return true;
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
        std::cerr << "Connection failed\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    uint32_t threads = htonl(2);
    std::vector<char> thread_data(reinterpret_cast<char*>(&threads), reinterpret_cast<char*>(&threads) + 4);
    if (!send_tlv(sock, 0x01, thread_data)) {
        std::cerr << "Failed to send thread count\n";
    }

    uint32_t rows = 3, cols = 4;
    std::vector<int32_t> matrix = {
        1, 2, 3, 6,
        4, 5, 6, 6,
        7, 8, 9, 6
    };

    std::vector<char> matrix_data;
    uint32_t net_rows = htonl(rows);
    uint32_t net_cols = htonl(cols);
    matrix_data.insert(matrix_data.end(), reinterpret_cast<char*>(&net_rows), reinterpret_cast<char*>(&net_rows) + 4);
    matrix_data.insert(matrix_data.end(), reinterpret_cast<char*>(&net_cols), reinterpret_cast<char*>(&net_cols) + 4);
    matrix_data.insert(matrix_data.end(), reinterpret_cast<char*>(matrix.data()),
        reinterpret_cast<char*>(matrix.data()) + matrix.size() * sizeof(int32_t));

    if (!send_tlv(sock, 0x02, matrix_data)) {
        std::cerr << "Failed to send matrix\n";
    }

    if (!send_tlv(sock, 0x03, {})) {
        std::cerr << "Failed to send start command\n";
    }
    if (!send_tlv(sock, 0x05, {})) {
        std::cerr << "Failed to send matrix status request\n";
    }
    else {
        char header[5];
        if (recv_all(sock, header, 5) && header[0] == 0x06 &&
            ((header[1] | header[2] | header[3]) == 0) && header[4] == 2) {

            char status[2];
            if (recv_all(sock, status, 2)) {
                std::cout << "Matrix is " << (status[0] ? "loaded" : "not loaded") << ", ";
                std::cout << "Processing is " << (status[1] ? "in progress" : "idle") << ".\n";
            }
            else {
                std::cerr << "Failed to receive matrix status\n";
            }
        }
        else {
            std::cerr << "Invalid matrix status response\n";
        }
    }

    std::vector<int32_t> result;
    uint32_t result_rows = 0, result_cols = 0;
    if (receive_result(sock, result, result_rows, result_cols)) {
        std::cout << "Received result matrix (" << result_rows << "x" << result_cols << "):\n";
        for (uint32_t i = 0; i < result_rows; ++i) {
            for (uint32_t j = 0; j < result_cols; ++j) {
                std::cout << result[i * result_cols + j] << "\t";
            }
            std::cout << "\n";
        }
    }
    else {
        std::cerr << "Failed to receive result\n";
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
