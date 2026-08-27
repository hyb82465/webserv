import socket
import time

HOST = "127.0.0.1"
PORT = 8080

s = socket.create_connection((HOST, PORT))

part1 = (
    b"POST /upload/partial.txt HTTP/1.1\r\n"
    b"Host: localhost\r\n"
    b"Transfer-Encoding: chunked\r\n"
    b"\r\n"
    b"5\r\n"
    b"Hel"
)

part2 = (
    b"lo\r\n"
    b"6\r\n"
    b" World\r\n"
    b"0\r\n"
    b"\r\n"
)

print("sending part 1...")
s.sendall(part1)

time.sleep(1)

print("sending part 2...")
s.sendall(part2)

response = b""

while True:
    data = s.recv(4096)

    if not data:
        break

    response += data

print(response.decode(errors="replace"))

s.close()
