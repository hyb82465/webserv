#!/usr/bin/env python3

import socket
import sys
import time


HOST = "127.0.0.1"
PORT = 8080
SERVER_IDLE_TIMEOUT = 30
WAIT_MARGIN = 5


def server_is_healthy():
    request = (
        b"GET / HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"Connection: close\r\n"
        b"\r\n"
    )
    try:
        sock = socket.create_connection((HOST, PORT), timeout=3)
        sock.settimeout(3)
        sock.sendall(request)
        response = b""
        while True:
            data = sock.recv(4096)
            if not data:
                break
            response += data
        sock.close()
        return response.startswith(b"HTTP/1.1 200 ")
    except OSError:
        return False


def main():
    sock = None
    try:
        sock = socket.create_connection((HOST, PORT), timeout=3)
        sock.sendall(
            b"GET / HTTP/1.1\r\n"
            b"Host: localhost\r\n"
        )
        time.sleep(SERVER_IDLE_TIMEOUT + WAIT_MARGIN)
        sock.settimeout(3)
        data = sock.recv(1024)
        closed = (data == b"")
    except ConnectionResetError:
        closed = True
    except socket.timeout:
        print("[FAIL] inactive connection remained open after %d seconds"
              % (SERVER_IDLE_TIMEOUT + WAIT_MARGIN))
        return 1
    except OSError as error:
        print("[FAIL] client timeout test: %s" % error)
        return 1
    finally:
        if sock is not None:
            sock.close()

    if not closed:
        print("[FAIL] server sent unexpected data to inactive client: %r" % data)
        return 1
    if not server_is_healthy():
        print("[FAIL] connection timed out, but server no longer answers GET /")
        return 1

    print("[PASS] inactive client was closed and server remained available")
    return 0


if __name__ == "__main__":
    sys.exit(main())
