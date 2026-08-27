import socket

HOST = "127.0.0.1"
PORT = 8080

s = socket.create_connection((HOST, PORT))
s.settimeout(3)

request1 = (
    b"GET / HTTP/1.1\r\n"
    b"Host: localhost\r\n"
    b"\r\n"
)

print("sending request 1...")
s.sendall(request1)

response1 = s.recv(4096)

print("response 1:")
print(response1.decode(errors="replace"))

request2 = (
    b"GET / HTTP/1.1\r\n"
    b"Host: localhost\r\n"
    b"Connection: close\r\n"
    b"\r\n"
)

print("sending request 2 on SAME socket...")
s.sendall(request2)

response2 = b""

while True:
    data = s.recv(4096)
    if not data:
        break
    response2 += data

print("response 2:")
print(response2.decode(errors="replace"))

s.close()
