#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <thread>
#include <mutex>
#include <iostream>
#include <cstdint>
#include "matrix_logic.h"

#pragma comment(lib, "ws2_32.lib")

bool recv_all(SOCKET socket, char* buffer, int length) {
    int total_received = 0;
    while (total_received < length) {
        int bytes = recv(socket, buffer + total_received, length - total_received, 0);
        if (bytes <= 0) return false;
        total_received += bytes;
    }
    return true;
}

bool send_all(SOCKET socket, const char* buffer, int length) {
    int total_sent = 0;
    while (total_sent < length) {
        int bytes = send(socket, buffer + total_sent, length - total_sent, 0);
        if (bytes <= 0) return false;
        total_sent += bytes;
    }
    return true;
}

inline bool receive_tlv(SOCKET client_socket, uint8_t& type, std::vector<char>& value) {
    char header[5];
    if (!recv_all(client_socket, header, 5)) return false;

    type = static_cast<uint8_t>(header[0]);

    uint32_t length =
        (static_cast<uint8_t>(header[1]) << 24) |
        (static_cast<uint8_t>(header[2]) << 16) |
        (static_cast<uint8_t>(header[3]) << 8) |
        (static_cast<uint8_t>(header[4]));

    std::cout << "[Server] Received TLV header: type = " << (int)type << ", length = " << length << std::endl;

    value.resize(length);
    return recv_all(client_socket, value.data(), length);
}

inline bool send_tlv(SOCKET client_socket, uint8_t type, const std::vector<char>& value) {
    uint32_t length = static_cast<uint32_t>(value.size());
    char header[5];
    header[0] = static_cast<char>(type);
    header[1] = (length >> 24) & 0xFF;
    header[2] = (length >> 16) & 0xFF;
    header[3] = (length >> 8) & 0xFF;
    header[4] = length & 0xFF;

    if (!send_all(client_socket, header, 5)) return false;
    return send_all(client_socket, value.data(), length);
}

// Допоміжна функція для надсилання повідомлень про помилку
void send_error(SOCKET client_socket, const std::string& msg) {
    std::vector<char> error_msg(msg.begin(), msg.end());
    send_tlv(client_socket, 0xFF, error_msg);
}


void handle_client(SOCKET client_socket) {
    std::cout << "[Server] Client connected." << std::endl;

    uint8_t type;
    std::vector<char> value;
    int num_threads = 0;
    std::vector<std::vector<int>> matrix;
    bool has_threads = false;
    bool has_matrix = false;

    while (receive_tlv(client_socket, type, value)) {
        try {
            if (type == 0x01) {
                if (value.size() != 4) {
                    send_error(client_socket, "Invalid thread count format.");
                    continue;
                }

                num_threads = (static_cast<uint8_t>(value[0]) << 24) |
                    (static_cast<uint8_t>(value[1]) << 16) |
                    (static_cast<uint8_t>(value[2]) << 8) |
                    (static_cast<uint8_t>(value[3]));

                if (num_threads <= 0) {
                    send_error(client_socket, "Thread count must be > 0.");
                    continue;
                }

                has_threads = true;
            }
            else if (type == 0x02) {
                if (value.size() < 8) {
                    send_error(client_socket, "Invalid matrix header.");
                    continue;
                }

                int rows = (static_cast<uint8_t>(value[0]) << 24) |
                    (static_cast<uint8_t>(value[1]) << 16) |
                    (static_cast<uint8_t>(value[2]) << 8) |
                    (static_cast<uint8_t>(value[3]));
                int cols = (static_cast<uint8_t>(value[4]) << 24) |
                    (static_cast<uint8_t>(value[5]) << 16) |
                    (static_cast<uint8_t>(value[6]) << 8) |
                    (static_cast<uint8_t>(value[7]));

                if (rows <= 0 || cols <= 1 || value.size() != 8 + rows * cols * 4) {
                    send_error(client_socket, "Invalid matrix size or format.");
                    continue;
                }

                matrix.resize(rows, std::vector<int>(cols));
                const char* data = value.data() + 8;

                for (int i = 0; i < rows; ++i) {
                    for (int j = 0; j < cols; ++j) {
                        int offset = (i * cols + j) * 4;
                        matrix[i][j] = (static_cast<uint8_t>(data[offset]) << 24) |
                            (static_cast<uint8_t>(data[offset + 1]) << 16) |
                            (static_cast<uint8_t>(data[offset + 2]) << 8) |
                            (static_cast<uint8_t>(data[offset + 3]));
                    }
                }

                has_matrix = true;
            }
            else if (type == 0x03) {
                if (!has_threads) {
                    send_error(client_socket, "Thread count not received.");
                    continue;
                }
                if (!has_matrix) {
                    send_error(client_socket, "Matrix data not received.");
                    continue;
                }

                if (matrix_processing_in_progress) {
                    send_error(client_socket, "Matrix is already being processed.");
                    continue;
                }

                matrix_processing_in_progress = true;

                std::thread([client_socket, &matrix, num_threads]() mutable {
                    process_matrix(matrix, num_threads);
                    matrix_processing_in_progress = false;

                    int rows = matrix.size();
                    int cols = matrix[0].size();
                    std::vector<char> result(8 + rows * cols * 4);
                    result[0] = (rows >> 24) & 0xFF;
                    result[1] = (rows >> 16) & 0xFF;
                    result[2] = (rows >> 8) & 0xFF;
                    result[3] = rows & 0xFF;
                    result[4] = (cols >> 24) & 0xFF;
                    result[5] = (cols >> 16) & 0xFF;
                    result[6] = (cols >> 8) & 0xFF;
                    result[7] = cols & 0xFF;

                    char* ptr = result.data() + 8;
                    for (int i = 0; i < rows; ++i) {
                        for (int j = 0; j < cols; ++j) {
                            int offset = (i * cols + j) * 4;
                            ptr[offset] = (matrix[i][j] >> 24) & 0xFF;
                            ptr[offset + 1] = (matrix[i][j] >> 16) & 0xFF;
                            ptr[offset + 2] = (matrix[i][j] >> 8) & 0xFF;
                            ptr[offset + 3] = matrix[i][j] & 0xFF;
                        }
                    }

                    send_tlv(client_socket, 0x04, result);
                    }).detach();    
            }
            else if (type == 0x05) {
                std::vector<char> status(2);
                status[0] = has_matrix ? 1 : 0;
                status[1] = matrix_processing_in_progress ? 1 : 0;
                send_tlv(client_socket, 0x06, status);  // TLV-відповідь на запит стану
            }

            else {
                send_error(client_socket, "Unknown TLV type.");
            }
        }
        catch (const std::exception& ex) {
            send_error(client_socket, std::string("Server error: ") + ex.what());
        }
        catch (...) {
            send_error(client_socket, "Unknown server error occurred.");
        }
    }

    std::cout << "[Server] Client disconnected." << std::endl;
    closesocket(client_socket);
}

void start_server(uint16_t port) {
    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_socket, SOMAXCONN);

    std::cout << "[Server] Listening on port " << port << std::endl;

    while (true) {
        SOCKET client_socket = accept(server_socket, nullptr, nullptr);
        std::thread(handle_client, client_socket).detach();
    }

    closesocket(server_socket);
    WSACleanup();
}
