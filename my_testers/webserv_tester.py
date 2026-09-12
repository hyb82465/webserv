import socket
import time
import os


DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 8080
TIMEOUT = 2


# ============================================================
# BASIC NETWORK
# ============================================================

def send_request(request, host=DEFAULT_HOST, port=DEFAULT_PORT):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(TIMEOUT)

    response = b""

    try:
        sock.connect((host, port))

        if isinstance(request, str):
            request = request.encode()

        sock.sendall(request)

        while True:
            try:
                data = sock.recv(4096)
            except socket.timeout:
                break

            if not data:
                break

            response += data

    except socket.error as error:
        print("[SOCKET ERROR]", error)

    sock.close()

    return response


# ============================================================
# SEND REQUEST IN MULTIPLE PIECES
# ============================================================

def send_parts(parts, host=DEFAULT_HOST, port=DEFAULT_PORT, delay=0.1):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(TIMEOUT)

    response = b""

    try:
        sock.connect((host, port))

        for part in parts:
            if isinstance(part, str):
                part = part.encode()

            sock.sendall(part)

            time.sleep(delay)

        while True:
            try:
                data = sock.recv(4096)
            except socket.timeout:
                break

            if not data:
                break

            response += data

    except socket.error as error:
        print("[SOCKET ERROR]", error)

    sock.close()

    return response


# ============================================================
# RESPONSE PARSING
# ============================================================

def response_to_string(response):
    return response.decode(errors="replace")


def get_status(response):
    if not response:
        return -1

    text = response_to_string(response)

    first_line = text.split("\r\n")[0]
    parts = first_line.split(" ")

    if len(parts) < 2:
        return -1

    try:
        return int(parts[1])
    except ValueError:
        return -1


def get_first_line(response):
    if not response:
        return ""

    text = response_to_string(response)

    return text.split("\r\n")[0]


def get_header(response, header_name):
    if not response:
        return None

    text = response_to_string(response)

    header_end = text.find("\r\n\r\n")

    if header_end == -1:
        header_text = text
    else:
        header_text = text[:header_end]

    lines = header_text.split("\r\n")

    for line in lines[1:]:
        if ":" not in line:
            continue

        name, value = line.split(":", 1)

        if name.lower() == header_name.lower():
            return value.strip()

    return None


def get_body(response):
    separator = b"\r\n\r\n"

    pos = response.find(separator)

    if pos == -1:
        return b""

    return response[pos + 4:]


# ============================================================
# TEST HELPERS
# ============================================================

passed = 0
failed = 0


def pass_test(name):
    global passed

    passed += 1

    print("[PASS]", name)


def fail_test(name, message=""):
    global failed

    failed += 1

    print("[FAIL]", name)

    if message:
        print("      ", message)


def test_status(name, request, expected_status,
                host=DEFAULT_HOST, port=DEFAULT_PORT):

    response = send_request(request, host, port)

    status = get_status(response)

    if status == expected_status:
        pass_test(name)
    else:
        fail_test(
            name,
            "expected " + str(expected_status)
            + ", got " + str(status)
            + " (" + get_first_line(response) + ")"
        )

    return response


def test_not_status(name, request, forbidden_status,
                    host=DEFAULT_HOST, port=DEFAULT_PORT):

    response = send_request(request, host, port)

    status = get_status(response)

    if status != forbidden_status:
        pass_test(name)
    else:
        fail_test(
            name,
            "should not return " + str(forbidden_status)
        )

    return response


def test_header(name, request,
                header_name, expected_value,
                host=DEFAULT_HOST, port=DEFAULT_PORT):

    response = send_request(request, host, port)

    value = get_header(response, header_name)

    if value == expected_value:
        pass_test(name)
    else:
        fail_test(
            name,
            "expected "
            + header_name
            + ": "
            + expected_value
            + ", got "
            + str(value)
        )

    return response


def info_test(name, request,
              host=DEFAULT_HOST, port=DEFAULT_PORT):

    response = send_request(request, host, port)

    print("[INFO]", name)

    if response:
        print("       ", get_first_line(response))
    else:
        print("        no response")

    return response


# ============================================================
# BASIC GET
# ============================================================

request = (
    "GET / HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "Connection: close\r\n"
    "\r\n"
)

test_status(
    "GET /",
    request,
    200
)


# ============================================================
# 404
# ============================================================

request = (
    "GET /does_not_exist HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "Connection: close\r\n"
    "\r\n"
)

test_status(
    "GET missing file",
    request,
    404
)


# ============================================================
# METHOD NOT ALLOWED
# ============================================================

request = (
    "POST / HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "Content-Length: 0\r\n"
    "Connection: close\r\n"
    "\r\n"
)

test_status(
    "POST / not allowed",
    request,
    405
)


request = (
    "DELETE / HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "Connection: close\r\n"
    "\r\n"
)

test_status(
    "DELETE / not allowed",
    request,
    405
)


# ============================================================
# HTTP VERSION
# ============================================================

request = (
    "GET / HTTP/2.0\r\n"
    "Host: localhost\r\n"
    "Connection: close\r\n"
    "\r\n"
)

test_status(
    "HTTP/2.0 unsupported",
    request,
    505
)


request = (
    "GET / HTTP/1.0\r\n"
    "Host: localhost\r\n"
    "Connection: close\r\n"
    "\r\n"
)

info_test(
    "HTTP/1.0 behavior",
    request
)


# ============================================================
# REDIRECT
# ============================================================

request = (
    "GET /old HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "Connection: close\r\n"
    "\r\n"
)

test_status(
    "GET /old redirect",
    request,
    301
)

test_header(
    "GET /old Location header",
    request,
    "Location",
    "/new.html"
)


# ============================================================
# IMAGES
# ============================================================

request = (
    "GET /images HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "Connection: close\r\n"
    "\r\n"
)

test_status(
    "GET /images",
    request,
    301
)


request = (
    "GET /images/ HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "Connection: close\r\n"
    "\r\n"
)

test_status(
    "GET /images/",
    request,
    200
)


# ============================================================
# RESPONSE CONTENT-LENGTH CHECK
# ============================================================

request = (
    "GET / HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "Connection: close\r\n"
    "\r\n"
)

response = send_request(request)

content_length = get_header(response, "Content-Length")
body = get_body(response)

if content_length is None:
    fail_test(
        "GET / has Content-Length",
        "header missing"
    )

else:
    try:
        expected = int(content_length)

        if expected == len(body):
            pass_test("GET / Content-Length matches body")
        else:
            fail_test(
                "GET / Content-Length matches body",
                "header = "
                + str(expected)
                + ", actual = "
                + str(len(body))
            )

    except ValueError:
        fail_test(
            "GET / valid Content-Length",
            "invalid value: " + content_length
        )


# ============================================================
# MAX BODY SIZE
#
# 127.0.0.2:8080
# client_max_body_size 5
# ============================================================

body = "123456"

request = (
    "POST / HTTP/1.1\r\n"
    "Host: 127.0.0.2\r\n"
    "Content-Length: 6\r\n"
    "Connection: close\r\n"
    "\r\n"
    + body
)

test_status(
    "body 6 > max 5",
    request,
    413,
    "127.0.0.2",
    8080
)


body = "12345"

request = (
    "POST / HTTP/1.1\r\n"
    "Host: 127.0.0.2\r\n"
    "Content-Length: 5\r\n"
    "Connection: close\r\n"
    "\r\n"
    + body
)

test_not_status(
    "body 5 == max 5",
    request,
    413,
    "127.0.0.2",
    8080
)


body = "1234"

request = (
    "POST / HTTP/1.1\r\n"
    "Host: 127.0.0.2\r\n"
    "Content-Length: 4\r\n"
    "Connection: close\r\n"
    "\r\n"
    + body
)

test_not_status(
    "body 4 < max 5",
    request,
    413,
    "127.0.0.2",
    8080
)


# ============================================================
# SECOND MAX BODY SIZE
#
# 127.0.0.3:8081
# max = 100
# ============================================================

body = "a" * 101

request = (
    "POST / HTTP/1.1\r\n"
    "Host: 127.0.0.3\r\n"
    "Content-Length: 101\r\n"
    "Connection: close\r\n"
    "\r\n"
    + body
)

test_status(
    "body 101 > max 100",
    request,
    413,
    "127.0.0.3",
    8081
)


body = "a" * 100

request = (
    "POST / HTTP/1.1\r\n"
    "Host: 127.0.0.3\r\n"
    "Content-Length: 100\r\n"
    "Connection: close\r\n"
    "\r\n"
    + body
)

test_not_status(
    "body 100 == max 100",
    request,
    413,
    "127.0.0.3",
    8081
)


# ============================================================
# MULTIPART UPLOAD
# ============================================================

boundary = "----WebservBoundary"

multipart_body = (
    "--" + boundary + "\r\n"
    'Content-Disposition: form-data; name="file"; filename="python_test.txt"\r\n'
    "Content-Type: text/plain\r\n"
    "\r\n"
    "hello from python tester\n"
    "\r\n"
    "--" + boundary + "--\r\n"
)

request = (
    "POST /upload HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "Content-Type: multipart/form-data; boundary=" + boundary + "\r\n"
    "Content-Length: " + str(len(multipart_body.encode())) + "\r\n"
    "Connection: close\r\n"
    "\r\n"
    + multipart_body
)

response = send_request(request)

status = get_status(response)

if status == 200 or status == 201:
    pass_test("multipart upload")
else:
    fail_test(
        "multipart upload",
        get_first_line(response)
    )


# ============================================================
# CHECK UPLOADED FILE
# ============================================================

upload_path = "./uploads/python_test.txt"

if os.path.isfile(upload_path):
    pass_test("multipart file created")

    try:
        file = open(upload_path, "r")
        content = file.read()
        file.close()

        if content == "hello from python tester\n":
            pass_test("multipart file content")
        else:
            fail_test(
                "multipart file content",
                "unexpected content"
            )

    except IOError as error:
        fail_test(
            "read uploaded file",
            str(error)
        )

else:
    fail_test(
        "multipart file created",
        upload_path + " does not exist"
    )


# ============================================================
# MULTIPART WITHOUT BOUNDARY
# ============================================================

request = (
    "POST /upload HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "Content-Type: multipart/form-data\r\n"
    "Content-Length: 5\r\n"
    "Connection: close\r\n"
    "\r\n"
    "hello"
)

test_status(
    "multipart without boundary",
    request,
    400
)


# ============================================================
# UNSAFE FILENAME
#
# 根据你的 isSafeFilename()
# 应该拒绝 ../
# ============================================================

boundary = "abc123"

multipart_body = (
    "--" + boundary + "\r\n"
    'Content-Disposition: form-data; name="file"; filename="../evil.txt"\r\n'
    "\r\n"
    "evil"
    "\r\n"
    "--" + boundary + "--\r\n"
)

request = (
    "POST /upload HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "Content-Type: multipart/form-data; boundary=" + boundary + "\r\n"
    "Content-Length: " + str(len(multipart_body.encode())) + "\r\n"
    "Connection: close\r\n"
    "\r\n"
    + multipart_body
)

test_status(
    "unsafe upload filename ../evil.txt",
    request,
    400
)


# ============================================================
# REQUEST SPLIT INTO MULTIPLE TCP SENDS
# ============================================================

parts = [
    "GET / HTTP/1.1\r\n",
    "Host: local",
    "host\r\n",
    "Connection: close\r\n",
    "\r\n"
]

response = send_parts(parts)

if get_status(response) == 200:
    pass_test("split request across multiple sends")
else:
    fail_test(
        "split request across multiple sends",
        get_first_line(response)
    )


# ============================================================
# SPLIT REQUEST LINE
# ============================================================

parts = [
    "GET / HT",
    "TP/1.",
    "1\r\n",
    "Host: localhost\r\n",
    "Connection: close\r\n",
    "\r\n"
]

response = send_parts(parts)

if get_status(response) == 200:
    pass_test("split request line")
else:
    fail_test(
        "split request line",
        get_first_line(response)
    )


# ============================================================
# SPLIT BODY
# ============================================================

parts = [
    "POST /upload HTTP/1.1\r\n",
    "Host: localhost\r\n",
    "Content-Length: 5\r\n",
    "Connection: close\r\n",
    "\r\n",
    "he",
    "llo"
]

response = send_parts(parts)

print("[INFO] split POST body")
print("       ", get_first_line(response))


# ============================================================
# EMPTY REQUEST
# ============================================================

response = send_request("")

print("[INFO] empty request")

if response:
    print("       ", get_first_line(response))
else:
    print("        no response / timeout")


# ============================================================
# INVALID REQUEST LINE
# ============================================================

request = (
    "HELLO\r\n"
    "Host: localhost\r\n"
    "Connection: close\r\n"
    "\r\n"
)

info_test(
    "invalid request line",
    request
)


# ============================================================
# MISSING HTTP VERSION
# ============================================================

request = (
    "GET /\r\n"
    "Host: localhost\r\n"
    "Connection: close\r\n"
    "\r\n"
)

info_test(
    "missing HTTP version",
    request
)


# ============================================================
# UNKNOWN METHOD
# ============================================================

request = (
    "BANANA / HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "Connection: close\r\n"
    "\r\n"
)

info_test(
    "unknown method BANANA",
    request
)


# ============================================================
# MISSING HOST
#
# HTTP/1.1 requires Host
# ============================================================

request = (
    "GET / HTTP/1.1\r\n"
    "Connection: close\r\n"
    "\r\n"
)

info_test(
    "HTTP/1.1 missing Host",
    request
)


# ============================================================
# EMPTY HOST
# ============================================================

request = (
    "GET / HTTP/1.1\r\n"
    "Host:\r\n"
    "Connection: close\r\n"
    "\r\n"
)

info_test(
    "empty Host header",
    request
)


# ============================================================
# INVALID HEADER
# ============================================================

request = (
    "GET / HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "BadHeaderWithoutColon\r\n"
    "Connection: close\r\n"
    "\r\n"
)

info_test(
    "invalid header without colon",
    request
)


# ============================================================
# DUPLICATE HOST
# ============================================================

request = (
    "GET / HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "Host: example.com\r\n"
    "Connection: close\r\n"
    "\r\n"
)

info_test(
    "duplicate Host header",
    request
)


# ============================================================
# CONTENT LENGTH = 0
# ============================================================

request = (
    "POST / HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "Content-Length: 0\r\n"
    "Connection: close\r\n"
    "\r\n"
)

test_status(
    "POST / Content-Length 0",
    request,
    405
)


# ============================================================
# INVALID CONTENT LENGTH
# ============================================================

request = (
    "POST / HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "Content-Length: abc\r\n"
    "Connection: close\r\n"
    "\r\n"
)

info_test(
    "invalid Content-Length abc",
    request
)


# ============================================================
# NEGATIVE CONTENT LENGTH
# ============================================================

request = (
    "POST / HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "Content-Length: -1\r\n"
    "Connection: close\r\n"
    "\r\n"
)

info_test(
    "negative Content-Length",
    request
)


# ============================================================
# BODY SMALLER THAN CONTENT-LENGTH
#
# 服务器应该等待剩余 body。
# tester 会 timeout。
# 最重要的是服务器不能 crash。
# ============================================================

request = (
    "POST /upload HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "Content-Length: 100\r\n"
    "Connection: close\r\n"
    "\r\n"
    "hello"
)

response = send_request(request)

print("[INFO] incomplete body")
if response:
    print("       ", get_first_line(response))
else:
    print("        no response / waiting for remaining body")


# ============================================================
# BODY LONGER THAN CONTENT LENGTH
# ============================================================

request = (
    "POST /upload HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "Content-Length: 5\r\n"
    "Connection: close\r\n"
    "\r\n"
    "helloEXTRA"
)

info_test(
    "body longer than Content-Length",
    request
)


# ============================================================
# QUERY STRING
# ============================================================

request = (
    "GET /?hello=world HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "Connection: close\r\n"
    "\r\n"
)

info_test(
    "GET with query string",
    request
)


# ============================================================
# VERY LONG PATH
# ============================================================

long_path = "/" + ("a" * 5000)

request = (
    "GET " + long_path + " HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "Connection: close\r\n"
    "\r\n"
)

info_test(
    "very long URI",
    request
)


# ============================================================
# CASE INSENSITIVE HEADER NAME
# ============================================================

request = (
    "GET / HTTP/1.1\r\n"
    "hOsT: localhost\r\n"
    "cOnNeCtIoN: close\r\n"
    "\r\n"
)

test_status(
    "mixed-case header names",
    request,
    200
)


# ============================================================
# SUMMARY
# ============================================================

print()
print("========================================")
print("TEST SUMMARY")
print("========================================")
print("PASS:", passed)
print("FAIL:", failed)
print("TOTAL:", passed + failed)

if failed == 0:
    print("ALL CHECKED TESTS PASSED")
else:
    print("SOME TESTS FAILED")