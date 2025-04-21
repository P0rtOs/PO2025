import socket
import struct

def send_tlv(sock, tlv_type, value_bytes):
    length = len(value_bytes)
    header = struct.pack("!BI", tlv_type, length)
    sock.sendall(header + value_bytes)

def recv_all(sock, length):
    data = b''
    while len(data) < length:
        more = sock.recv(length - len(data))
        if not more:
            raise ConnectionError("Socket closed")
        data += more
    return data

def receive_tlv(sock):
    header = recv_all(sock, 5)
    tlv_type, length = struct.unpack("!BI", header)
    value = recv_all(sock, length)
    return tlv_type, value

HOST = '127.0.0.1'
PORT = 12345

with socket.create_connection((HOST, PORT)) as sock:
    # Надсилаємо матрицю (тип 0x02) без кількості потоків — це викликає помилку
    rows, cols = 2, 3
    matrix = [
        [1, 2, 3],
        [4, 5, 6],
    ]
    matrix_data = struct.pack("!II", rows, cols)
    for row in matrix:
        for num in row:
            matrix_data += struct.pack("!i", num)
    send_tlv(sock, 0x02, matrix_data)

    # Без типу 0x01 — одразу запускаємо обчислення
    send_tlv(sock, 0x03, b'')

    # Читаємо відповідь
    tlv_type, value = receive_tlv(sock)
    if tlv_type == 0xFF:
        print("[ERROR from server]:", value.decode())
    else:
        print("Unexpected reply, type:", tlv_type)
