#!/usr/bin/env python3

import socket
import sys
import time

sock = None

try:
    sock = socket.create_connection(
        ("127.0.0.1", 8080),
        timeout=3
    )

    # 故意不发送 header 结束标志 \r\n\r\n。
    sock.sendall(
        b"GET / HTTP/1.1\r\n"
        b"Host: localhost\r\n"
    )

    print("Partial request sent.")
    print("Waiting 35 seconds for the server timeout...")

    time.sleep(35)

    sock.settimeout(3)
    data = sock.recv(1024)

    if data == b"":
        print("PASS: server closed the inactive connection")
        sys.exit(0)

    print("FAIL: server sent unexpected data:", repr(data))
    sys.exit(1)

except socket.timeout:
    print("FAIL: connection is still open after the timeout")
    sys.exit(1)

except OSError as error:
    print("FAIL: socket error:", type(error).__name__, error)
    sys.exit(1)

finally:
    if sock is not None:
        sock.close()
