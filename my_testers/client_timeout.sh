python3 - <<'PY'
import socket
import time

s = socket.create_connection(("127.0.0.1", 8080))

# 缺少最后的 \r\n，故意让 header 不完整
s.sendall(
    b"GET / HTTP/1.1\r\n"
    b"Host: localhost\r\n"
)

print("partial request sent")
print("waiting 35 seconds...")

time.sleep(35)
s.settimeout(3)

try:
    data = s.recv(1024)
    print("recv:", repr(data))
except Exception as e:
    print("connection result:", type(e).__name__, e)

s.close()
PY
