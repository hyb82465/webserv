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

request2 = (
    b"GET / HTTP/1.1\r\n"
    b"Host: localhost\r\n"
    b"Connection: close\r\n"
    b"\r\n"
)

print("sending TWO requests at once...")

s.sendall(request1 + request2)

response = b""

try:
    while True:
        data = s.recv(4096)
        if not data:
            break
        response += data
except socket.timeout:
    print("TIMEOUT: server did not finish both responses")

print()
print(response.decode(errors="replace"))

print()
print("number of responses:",
      response.count(b"HTTP/1.1 200 OK"))

s.close()
