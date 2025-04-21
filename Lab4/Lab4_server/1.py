import socket
import struct

def send_all(sock, data):
    total_sent = 0
    while total_sent < len(data):
        sent = sock.send(data[total_sent:])
        if sent == 0:
            raise RuntimeError("Socket connection broken")
        total_sent += sent

def recv_all(sock, length):
    data = b''
    while len(data) < length:
        chunk = sock.recv(length - len(data))
        if not chunk:
            raise RuntimeError("Socket connection broken")
        data += chunk
    return data

def send_tlv(sock, tlv_type, value_bytes):
    length = len(value_bytes)
    header = struct.pack("!B I", tlv_type, length)
    send_all(sock, header + value_bytes)

def receive_result(sock):
    header = recv_all(sock, 5)
    tlv_type, length = struct.unpack("!B I", header)
    if tlv_type != 0x04:
        raise ValueError("Unexpected TLV type")

    payload = recv_all(sock, length)
    rows, cols = struct.unpack("!II", payload[:8])
    matrix = []
    for i in range(rows * cols):
        val = struct.unpack("!i", payload[8 + i * 4: 8 + (i + 1) * 4])[0]
        matrix.append(val)

    return rows, cols, matrix

def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(("127.0.0.1", 12345))

    # 1. Надсилаємо кількість потоків (2)
    thread_count = struct.pack("!I", 2)
    send_tlv(sock, 0x01, thread_count)

    # 2. Надсилаємо матрицю 3x4
    rows, cols = 3, 4
    matrix = [
        1, 2, 3, 6,
        4, 5, 6, 6,
        7, 8, 9, 6
    ]

    matrix_data = struct.pack("!II", rows, cols)
    for val in matrix:
        matrix_data += struct.pack("!i", val)
    send_tlv(sock, 0x02, matrix_data)

    # 3. Надсилаємо команду запуску обчислень
    send_tlv(sock, 0x03, b'')

    # 4. Отримуємо результат
    result_rows, result_cols, result_matrix = receive_result(sock)
    print("Received result matrix ({}x{}):".format(result_rows, result_cols))
    for i in range(result_rows):
        for j in range(result_cols):
            print(result_matrix[i * result_cols + j], end='\t')
        print()

    sock.close()

if __name__ == "__main__":
    main()
