#!/usr/bin/env python3
# Subject-oriented black-box tester for 42 Webserv v24.1
# CGI intentionally excluded.
#
# Run from the project root while webserv is already running:
#     python3 tests/webserv_subject_no_cgi.py
#
# IMPORTANT:
# The CONFIG section below must match the configuration file used to start
# your server. Tests that depend on a route are only meaningful if that
# route really has the described configuration.

import socket
import time
import os
import threading


# =====================================================================
# CONFIG - ADAPT THESE VALUES TO THE CONFIG FILE USED BY WEBSERV
# =====================================================================

DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 8080
TIMEOUT = 1.5

# Your current test configuration, based on the tests you were already using.
ROOT_URL = "/"
ROOT_LOCAL_DIR = "./www"

UPLOAD_URL = "/upload"
UPLOAD_LOCAL_DIR = "./uploads"

REDIRECT_URL = "/old"
REDIRECT_EXPECTED_STATUS = 301
REDIRECT_EXPECTED_LOCATION = "/new"

# A route mapped to ./assets with GET allowed.
ROOT_MAPPING_URL = "/images"
ROOT_MAPPING_LOCAL_DIR = "./assets"

# Existing directory route.
DIRECTORY_URL = "/directory/"

# Routes whose allowed methods are known.
# Only put a route here if your actual config really forbids the method.
METHOD_TESTS = [
    ("POST / forbidden by route", "POST", "/", 405),
    ("DELETE / forbidden by route", "DELETE", "/", 405),
    ("POST /images forbidden by route", "POST", "/images", 405),
]

# client_max_body_size test endpoints from your current setup.
BODY_LIMITS = [
    # (host, port, Host header, max_size)
    ("127.0.0.2", 8080, "127.0.0.2", 5),
    ("127.0.0.3", 8081, "127.0.0.3", 100),
]

# Subject requires multiple interface:port pairs.
LISTEN_ENDPOINTS = [
    ("127.0.0.1", 8080),
    ("127.0.0.1", 8081),
    ("127.0.0.2", 8080),
    ("127.0.0.3", 8081),
]

# ---------------------------------------------------------------------
# DELETE MANDATORY FEATURE
# ---------------------------------------------------------------------
# Subject requires DELETE support.
#
# To test DELETE properly, you need ONE route in your config that allows
# DELETE and maps to a directory where this tester may create/delete a file.
#
# Example:
#   DELETE_URL_PREFIX = "/delete"
#   DELETE_LOCAL_DIR = "./www/delete"
#
# If your config does not currently expose a DELETE-enabled route, leave
# these as None. The tester will show a yellow SKIP and warn that mandatory
# DELETE coverage is missing.
DELETE_URL_PREFIX = "/delete"
DELETE_LOCAL_DIR = "./www"


# =====================================================================
# ANSI COLORS
# =====================================================================

USE_COLOR = os.environ.get("NO_COLOR") is None

def color(code, text):
    if not USE_COLOR:
        return text
    return "\033[" + code + "m" + text + "\033[0m"

GREEN = lambda s: color("92", s)
RED = lambda s: color("91", s)
YELLOW = lambda s: color("93", s)
CYAN = lambda s: color("96", s)
MAGENTA = lambda s: color("95", s)
BOLD = lambda s: color("1", s)


# =====================================================================
# COUNTERS
# =====================================================================

passed = 0
failed = 0
info_count = 0
skipped = 0


# =====================================================================
# NETWORK
# =====================================================================

def send_request(request, host=DEFAULT_HOST, port=DEFAULT_PORT,
                 timeout=TIMEOUT, shutdown_write=False):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(timeout)
    response = b""

    try:
        sock.connect((host, port))

        if isinstance(request, str):
            request = request.encode()

        if request:
            sock.sendall(request)

        if shutdown_write:
            try:
                sock.shutdown(socket.SHUT_WR)
            except socket.error:
                pass

        while True:
            try:
                data = sock.recv(4096)
            except socket.timeout:
                break

            if not data:
                break

            response += data

    except socket.error:
        pass

    try:
        sock.close()
    except socket.error:
        pass

    return response


def send_parts(parts, host=DEFAULT_HOST, port=DEFAULT_PORT,
               delay=0.03, timeout=TIMEOUT, shutdown_write=False):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(timeout)
    response = b""

    try:
        sock.connect((host, port))

        for part in parts:
            if isinstance(part, str):
                part = part.encode()

            if part:
                sock.sendall(part)

            if delay:
                time.sleep(delay)

        if shutdown_write:
            try:
                sock.shutdown(socket.SHUT_WR)
            except socket.error:
                pass

        while True:
            try:
                data = sock.recv(4096)
            except socket.timeout:
                break

            if not data:
                break

            response += data

    except socket.error:
        pass

    try:
        sock.close()
    except socket.error:
        pass

    return response


# =====================================================================
# RESPONSE PARSING
# =====================================================================

def response_to_string(response):
    return response.decode(errors="replace")


def split_response(response):
    pos = response.find(b"\r\n\r\n")

    if pos == -1:
        return response, b""

    return response[:pos], response[pos + 4:]


def get_first_line(response):
    if not response:
        return ""

    return response_to_string(response).split("\r\n", 1)[0]


def get_status(response):
    first_line = get_first_line(response)
    parts = first_line.split(" ")

    if len(parts) < 2:
        return -1

    try:
        return int(parts[1])
    except ValueError:
        return -1


def get_body(response):
    return split_response(response)[1]


def get_header(response, header_name):
    if not response:
        return None

    header_bytes, _ = split_response(response)
    text = header_bytes.decode("iso-8859-1", errors="replace")

    for line in text.split("\r\n")[1:]:
        if ":" not in line:
            continue

        name, value = line.split(":", 1)

        if name.strip().lower() == header_name.lower():
            return value.strip()

    return None


def get_all_headers(response, header_name):
    values = []

    if not response:
        return values

    header_bytes, _ = split_response(response)
    text = header_bytes.decode("iso-8859-1", errors="replace")

    for line in text.split("\r\n")[1:]:
        if ":" not in line:
            continue

        name, value = line.split(":", 1)

        if name.strip().lower() == header_name.lower():
            values.append(value.strip())

    return values


# =====================================================================
# OUTPUT
# =====================================================================

def section(name):
    print()
    print(CYAN("=" * 72))
    print(CYAN(BOLD(name)))
    print(CYAN("=" * 72))


def pass_test(name):
    global passed
    passed += 1
    print(GREEN("[PASS]"), name)


def fail_test(name, message=""):
    global failed
    failed += 1
    print(RED("[FAIL]"), name)

    if message:
        print(RED("       " + message))


def info_test(name, message=""):
    global info_count
    info_count += 1
    print(YELLOW("[INFO]"), name)

    if message:
        print(YELLOW("       " + message))


def skip_test(name, message=""):
    global skipped
    skipped += 1
    print(YELLOW("[SKIP]"), name)

    if message:
        print(YELLOW("       " + message))


def test_status(name, request, expected,
                host=DEFAULT_HOST, port=DEFAULT_PORT):
    response = send_request(request, host, port)
    status = get_status(response)

    if status == expected:
        pass_test(name)
    else:
        fail_test(
            name,
            "expected %d, got %d (%s)"
            % (expected, status, get_first_line(response))
        )

    return response


def health_check(name):
    response = send_request(
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Connection: close\r\n"
        "\r\n"
    )

    if get_status(response) == 200:
        pass_test(name)
        return True

    fail_test(name, get_first_line(response) or "no response")
    return False


def content_length_is_correct(response):
    value = get_header(response, "Content-Length")

    if value is None:
        return False, "Content-Length missing"

    try:
        expected = int(value)
    except ValueError:
        return False, "invalid Content-Length: " + value

    actual = len(get_body(response))

    if expected != actual:
        return False, "header=%d actual=%d" % (expected, actual)

    return True, ""


def make_request(method, url, body=b"", extra_headers=None,
                 host_header="localhost"):
    if isinstance(body, str):
        body = body.encode()

    headers = [
        "%s %s HTTP/1.1" % (method, url),
        "Host: " + host_header,
    ]

    if extra_headers:
        headers.extend(extra_headers)

    if method in ("POST", "PUT", "PATCH") and not any(
        h.lower().startswith("content-length:") for h in headers
    ):
        headers.append("Content-Length: %d" % len(body))

    headers.append("Connection: close")

    head = "\r\n".join(headers) + "\r\n\r\n"

    return head.encode() + body


# =====================================================================
# SUBJECT 1 - STATIC WEBSITE + GET
# =====================================================================

section("SUBJECT 1 - GET / STATIC WEBSITE")

marker_name = "__webserv_subject_static__.txt"
marker_path = os.path.join(ROOT_LOCAL_DIR, marker_name)
marker_url = "/" + marker_name
marker_content = b"42-webserv-static-file-test\n"

try:
    if not os.path.isdir(ROOT_LOCAL_DIR):
        fail_test("static website root exists",
                  ROOT_LOCAL_DIR + " does not exist")
    else:
        pass_test("static website root exists")

        with open(marker_path, "wb") as f:
            f.write(marker_content)

        response = test_status(
            "GET static file",
            make_request("GET", marker_url),
            200
        )

        if get_body(response) == marker_content:
            pass_test("static file body is exact")
        else:
            fail_test(
                "static file body is exact",
                "expected %r got %r"
                % (marker_content, get_body(response))
            )

        ok, message = content_length_is_correct(response)
        if ok:
            pass_test("static file Content-Length is accurate")
        else:
            fail_test("static file Content-Length is accurate", message)

except OSError as error:
    fail_test("prepare static-file test", str(error))


# =====================================================================
# SUBJECT 2 - 404 + DEFAULT ERROR PAGE
# =====================================================================

section("SUBJECT 2 - ERROR RESPONSE / DEFAULT ERROR PAGE")

response = test_status(
    "missing resource returns 404",
    make_request("GET", "/__definitely_missing_webserv_file__"),
    404
)

if len(get_body(response)) > 0:
    pass_test("404 has an error page body")
else:
    fail_test("404 has an error page body", "empty response body")

ok, message = content_length_is_correct(response)
if ok:
    pass_test("404 Content-Length is accurate")
else:
    fail_test("404 Content-Length is accurate", message)


# =====================================================================
# SUBJECT 3 - CONFIGURED METHODS
# =====================================================================

section("SUBJECT 3 - ACCEPTED HTTP METHODS PER ROUTE")

for name, method, url, expected in METHOD_TESTS:
    response = test_status(
        name,
        make_request(method, url),
        expected
    )

# GET, POST, DELETE are required by subject.
# GET and POST are exercised elsewhere; DELETE needs an explicitly
# DELETE-enabled route to prove the feature exists.


# =====================================================================
# SUBJECT 4 - REDIRECTION
# =====================================================================

section("SUBJECT 4 - HTTP REDIRECTION")

response = test_status(
    "configured redirect status",
    make_request("GET", REDIRECT_URL),
    REDIRECT_EXPECTED_STATUS
)

location = get_header(response, "Location")

if location == REDIRECT_EXPECTED_LOCATION:
    pass_test("redirect Location header")
else:
    fail_test(
        "redirect Location header",
        "expected %r got %r"
        % (REDIRECT_EXPECTED_LOCATION, location)
    )


# =====================================================================
# SUBJECT 5 - ROOT MAPPING
# =====================================================================

section("SUBJECT 5 - ROUTE ROOT MAPPING")

mapped_name = "__webserv_root_mapping__.txt"
mapped_path = os.path.join(ROOT_MAPPING_LOCAL_DIR, mapped_name)
mapped_url = ROOT_MAPPING_URL.rstrip("/") + "/" + mapped_name
mapped_content = b"42-route-root-mapping-test\n"

try:
    if not os.path.isdir(ROOT_MAPPING_LOCAL_DIR):
        fail_test(
            "mapped root directory exists",
            ROOT_MAPPING_LOCAL_DIR + " does not exist"
        )
    else:
        with open(mapped_path, "wb") as f:
            f.write(mapped_content)

        response = test_status(
            "GET file through configured route root",
            make_request("GET", mapped_url),
            200
        )

        if get_body(response) == mapped_content:
            pass_test("route root maps URL to correct local file")
        else:
            fail_test(
                "route root maps URL to correct local file",
                "unexpected response body"
            )

except OSError as error:
    fail_test("prepare root-mapping test", str(error))


# =====================================================================
# SUBJECT 6 - DIRECTORY INDEX
# =====================================================================

section("SUBJECT 6 - DEFAULT FILE FOR DIRECTORY")

response = test_status(
    "GET / serves configured default index",
    make_request("GET", ROOT_URL),
    200
)

index_path = os.path.join(ROOT_LOCAL_DIR, "index.html")

if os.path.isfile(index_path):
    try:
        with open(index_path, "rb") as f:
            index_content = f.read()

        if get_body(response) == index_content:
            pass_test("GET / body equals configured index file")
        else:
            info_test(
                "GET / body differs from ./www/index.html",
                "This is only a problem if your config says index index.html."
            )
    except OSError as error:
        info_test("could not compare index file", str(error))
else:
    info_test(
        "index comparison skipped",
        index_path + " not found; adjust tester if your index file has another name."
    )


# =====================================================================
# SUBJECT 7 - DIRECTORY LISTING ENABLE / DISABLE
# =====================================================================

section("SUBJECT 7 - DIRECTORY LISTING")

# Autoindex ON:
# We create a directory with no index file under the route whose autoindex
# is expected to be enabled. The returned page should contain the marker
# filename.
auto_dir_name = "__webserv_autoindex_on__"
auto_dir_path = os.path.join(ROOT_MAPPING_LOCAL_DIR, auto_dir_name)
auto_marker_name = "visible_in_autoindex.txt"
auto_marker_path = os.path.join(auto_dir_path, auto_marker_name)
auto_url = ROOT_MAPPING_URL.rstrip("/") + "/" + auto_dir_name + "/"

try:
    os.makedirs(auto_dir_path, exist_ok=True)

    with open(auto_marker_path, "wb") as f:
        f.write(b"autoindex marker\n")

    response = test_status(
        "directory listing enabled returns 200",
        make_request("GET", auto_url),
        200
    )

    if auto_marker_name.encode() in get_body(response):
        pass_test("enabled directory listing contains file name")
    else:
        fail_test(
            "enabled directory listing contains file name",
            "marker filename not found in generated listing"
        )

except OSError as error:
    fail_test("prepare autoindex-on test", str(error))


# Autoindex OFF:
# Root location is expected to have autoindex disabled. Create a directory
# with no index and verify the server does not reveal its directory listing.
noindex_dir_name = "__webserv_autoindex_off__"
noindex_dir_path = os.path.join(ROOT_LOCAL_DIR, noindex_dir_name)
noindex_marker_name = "must_not_be_listed.txt"
noindex_marker_path = os.path.join(noindex_dir_path, noindex_marker_name)
noindex_url = "/" + noindex_dir_name + "/"

try:
    os.makedirs(noindex_dir_path, exist_ok=True)

    with open(noindex_marker_path, "wb") as f:
        f.write(b"hidden directory marker\n")

    response = send_request(make_request("GET", noindex_url))
    status = get_status(response)

    if status != 200:
        pass_test("directory listing disabled does not return listing")
    elif noindex_marker_name.encode() not in get_body(response):
        pass_test("directory listing disabled hides directory contents")
    else:
        fail_test(
            "directory listing disabled hides directory contents",
            "directory contents were exposed"
        )

except OSError as error:
    fail_test("prepare autoindex-off test", str(error))


# =====================================================================
# SUBJECT 8 - UPLOAD FILES
# =====================================================================

section("SUBJECT 8 - FILE UPLOAD")

def multipart_request(filename, content,
                      boundary="----42WebservSubjectBoundary"):
    if isinstance(content, str):
        content = content.encode()

    body = (
        ("--" + boundary + "\r\n").encode()
        + ('Content-Disposition: form-data; name="file"; filename="%s"\r\n'
           % filename).encode()
        + b"Content-Type: application/octet-stream\r\n"
        + b"\r\n"
        + content
        + b"\r\n"
        + ("--" + boundary + "--\r\n").encode()
    )

    return make_request(
        "POST",
        UPLOAD_URL,
        body,
        [
            "Content-Type: multipart/form-data; boundary=" + boundary,
            "Content-Length: %d" % len(body),
        ]
    )


upload_name = "__webserv_subject_upload__.bin"
upload_path = os.path.join(UPLOAD_LOCAL_DIR, upload_name)
upload_content = b"A\x00B\xffC\n42\n"

try:
    if os.path.isfile(upload_path):
        os.remove(upload_path)
except OSError:
    pass

response = send_request(
    multipart_request(upload_name, upload_content)
)

if get_status(response) in (200, 201, 204):
    pass_test("POST uploads a file")
else:
    fail_test(
        "POST uploads a file",
        "got " + get_first_line(response)
    )

if os.path.isfile(upload_path):
    pass_test("uploaded file exists in configured storage")

    try:
        with open(upload_path, "rb") as f:
            saved = f.read()

        if saved == upload_content:
            pass_test("uploaded binary file content is exact")
        else:
            fail_test(
                "uploaded binary file content is exact",
                "expected %r got %r" % (upload_content, saved)
            )

    except OSError as error:
        fail_test("read uploaded file", str(error))
else:
    fail_test(
        "uploaded file exists in configured storage",
        upload_path + " not found"
    )


# Missing multipart boundary: malformed upload must not crash the server.
response = send_request(
    make_request(
        "POST",
        UPLOAD_URL,
        b"hello",
        [
            "Content-Type: multipart/form-data",
            "Content-Length: 5",
        ]
    )
)

if get_status(response) == 400:
    pass_test("malformed multipart request returns 400")
else:
    info_test(
        "malformed multipart status",
        "got %s; subject mainly requires resilience here"
        % get_first_line(response)
    )

health_check("server alive after malformed upload")


# =====================================================================
# SUBJECT 9 - CLIENT_MAX_BODY_SIZE
# =====================================================================

section("SUBJECT 9 - CLIENT MAX BODY SIZE")

for host, port, host_header, limit in BODY_LIMITS:
    too_large = b"a" * (limit + 1)
    exact = b"a" * limit
    below = b"a" * max(0, limit - 1)

    response = test_status(
        "body %d > max %d returns 413 on %s:%d"
        % (limit + 1, limit, host, port),
        make_request(
            "POST",
            "/",
            too_large,
            host_header=host_header
        ),
        413,
        host,
        port
    )

    response = send_request(
        make_request(
            "POST",
            "/",
            exact,
            host_header=host_header
        ),
        host,
        port
    )

    if get_status(response) != 413:
        pass_test(
            "body %d == max %d is not rejected as too large"
            % (limit, limit)
        )
    else:
        fail_test(
            "body %d == max %d is not rejected as too large"
            % (limit, limit)
        )

    response = send_request(
        make_request(
            "POST",
            "/",
            below,
            host_header=host_header
        ),
        host,
        port
    )

    if get_status(response) != 413:
        pass_test(
            "body %d < max %d is not rejected as too large"
            % (len(below), limit)
        )
    else:
        fail_test(
            "body %d < max %d is not rejected as too large"
            % (len(below), limit)
        )


# =====================================================================
# SUBJECT 10 - DELETE METHOD
# =====================================================================

section("SUBJECT 10 - DELETE METHOD")

if DELETE_URL_PREFIX is None or DELETE_LOCAL_DIR is None:
    skip_test(
        "DELETE functional test",
        "Mandatory feature is NOT proven by this tester. "
        "Configure DELETE_URL_PREFIX and DELETE_LOCAL_DIR."
    )
else:
    delete_name = "__webserv_subject_delete__.txt"
    delete_path = os.path.join(DELETE_LOCAL_DIR, delete_name)
    delete_url = DELETE_URL_PREFIX.rstrip("/") + "/" + delete_name

    try:
        os.makedirs(DELETE_LOCAL_DIR, exist_ok=True)

        with open(delete_path, "wb") as f:
            f.write(b"delete me\n")

        response = send_request(make_request("DELETE", delete_url))
        status = get_status(response)

        if status in (200, 202, 204):
            pass_test("DELETE accepted on configured route")
        else:
            fail_test(
                "DELETE accepted on configured route",
                "got " + get_first_line(response)
            )

        if not os.path.exists(delete_path):
            pass_test("DELETE actually removes the file")
        else:
            fail_test(
                "DELETE actually removes the file",
                delete_path + " still exists"
            )

    except OSError as error:
        fail_test("prepare DELETE test", str(error))


# =====================================================================
# SUBJECT 11 - MULTIPLE INTERFACE:PORT PAIRS
# =====================================================================

section("SUBJECT 11 - MULTIPLE LISTEN ENDPOINTS")

for host, port in LISTEN_ENDPOINTS:
    response = send_request(
        make_request("GET", "/", host_header=host),
        host,
        port
    )

    if response and get_status(response) != -1:
        pass_test("listener reachable %s:%d" % (host, port))
    else:
        fail_test(
            "listener reachable %s:%d" % (host, port),
            "no valid HTTP response"
        )


# =====================================================================
# SUBJECT 12 - CLIENT DISCONNECTIONS
# =====================================================================

section("SUBJECT 12 - CLIENT DISCONNECTIONS")

try:
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(TIMEOUT)
    sock.connect((DEFAULT_HOST, DEFAULT_PORT))
    sock.close()
    pass_test("client can disconnect immediately")
except socket.error as error:
    fail_test("client can disconnect immediately", str(error))

health_check("server remains available after immediate disconnect")


# Incomplete headers + EOF.
send_request(
    b"GET / HTTP/1.1\r\nHost: localhost\r\n",
    shutdown_write=True
)
health_check("server remains available after incomplete header disconnect")

# Incomplete body + EOF.
send_request(
    b"POST /upload HTTP/1.1\r\n"
    b"Host: localhost\r\n"
    b"Content-Length: 100\r\n"
    b"Connection: close\r\n\r\n"
    b"hello",
    shutdown_write=True
)
health_check("server remains available after incomplete body disconnect")


# =====================================================================
# SUBJECT 13 - TCP FRAGMENTATION
# =====================================================================

section("SUBJECT 13 - REQUESTS SPLIT ACROSS RECV() CALLS")

response = send_parts([
    "GET / HTTP/1.1\r\n",
    "Host: local",
    "host\r\n",
    "Connection: close\r\n",
    "\r\n",
])

if get_status(response) == 200:
    pass_test("request survives TCP fragmentation")
else:
    fail_test(
        "request survives TCP fragmentation",
        get_first_line(response) or "no response"
    )

response = send_parts([
    "GET / HTTP/1.1\r",
    "\nHost: localhost\r",
    "\nConnection: close\r",
    "\n\r",
    "\n",
])

if get_status(response) == 200:
    pass_test("CRLF itself may be split between recv() calls")
else:
    fail_test(
        "CRLF itself may be split between recv() calls",
        get_first_line(response) or "no response"
    )


# =====================================================================
# SUBJECT 14 - NON-BLOCKING BEHAVIOUR (BLACK-BOX)
# =====================================================================

section("SUBJECT 14 - NON-BLOCKING / SLOW CLIENT TEST")

slow_sockets = []

try:
    # Keep many clients connected with an unfinished request.
    # A blocking server that waits inside recv() on one client will often
    # stop serving the healthy client below.
    for i in range(20):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(TIMEOUT)
        s.connect((DEFAULT_HOST, DEFAULT_PORT))
        s.sendall(
            b"GET / HTTP/1.1\r\n"
            b"Host: localhost\r\n"
            b"X-Slow: still-not-finished-"
        )
        slow_sockets.append(s)

    start = time.time()
    response = send_request(
        make_request("GET", "/"),
        timeout=1.0
    )
    elapsed = time.time() - start

    if get_status(response) == 200:
        pass_test("normal client works while 20 slow clients are pending")
    else:
        fail_test(
            "normal client works while 20 slow clients are pending",
            get_first_line(response) or "no response"
        )

    if elapsed < 1.0:
        pass_test("slow clients do not block normal request")
    else:
        fail_test(
            "slow clients do not block normal request",
            "normal request took %.3f seconds" % elapsed
        )

finally:
    for s in slow_sockets:
        try:
            s.close()
        except socket.error:
            pass

health_check("server remains available after slow-client test")


# Slow POST bodies.
slow_sockets = []

try:
    for i in range(10):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(TIMEOUT)
        s.connect((DEFAULT_HOST, DEFAULT_PORT))
        s.sendall(
            b"POST /upload HTTP/1.1\r\n"
            b"Host: localhost\r\n"
            b"Content-Length: 100000\r\n"
            b"Connection: keep-alive\r\n"
            b"\r\n"
            b"x"
        )
        slow_sockets.append(s)

    response = send_request(make_request("GET", "/"), timeout=1.0)

    if get_status(response) == 200:
        pass_test("normal GET works while 10 POST bodies are incomplete")
    else:
        fail_test(
            "normal GET works while 10 POST bodies are incomplete",
            get_first_line(response) or "no response"
        )

finally:
    for s in slow_sockets:
        try:
            s.close()
        except socket.error:
            pass

health_check("server remains available after slow-body clients")


# =====================================================================
# SUBJECT 15 - STRESS / AVAILABILITY
# =====================================================================

section("SUBJECT 15 - STRESS TEST")

STRESS_CLIENTS = 100
results = [False] * STRESS_CLIENTS


def stress_worker(index):
    response = send_request(
        make_request("GET", "/"),
        timeout=2.0
    )
    results[index] = (get_status(response) == 200)


threads = []
start = time.time()

for i in range(STRESS_CLIENTS):
    thread = threading.Thread(target=stress_worker, args=(i,))
    threads.append(thread)
    thread.start()

for thread in threads:
    thread.join()

elapsed = time.time() - start
successes = sum(1 for value in results if value)

if successes == STRESS_CLIENTS:
    pass_test("100 concurrent clients all receive 200")
else:
    fail_test(
        "100 concurrent clients all receive 200",
        "%d/%d succeeded" % (successes, STRESS_CLIENTS)
    )

info_test(
    "concurrent stress duration",
    "%.3f seconds for %d clients" % (elapsed, STRESS_CLIENTS)
)

health_check("server remains available after stress test")


# =====================================================================
# SUBJECT 16 - RESPONSE STATUS / FORMAT SANITY
# =====================================================================

section("SUBJECT 16 - HTTP RESPONSE SANITY")

response = send_request(make_request("GET", "/"))

if get_first_line(response).startswith("HTTP/1.1 "):
    pass_test("response has an HTTP/1.1 status line")
else:
    fail_test(
        "response has an HTTP/1.1 status line",
        get_first_line(response)
    )

if b"\r\n\r\n" in response:
    pass_test("response separates headers and body with CRLF CRLF")
else:
    fail_test("response separates headers and body with CRLF CRLF")

values = get_all_headers(response, "Content-Length")

if len(values) == 1:
    pass_test("response has one Content-Length")
else:
    fail_test(
        "response has one Content-Length",
        "found %d values: %r" % (len(values), values)
    )

ok, message = content_length_is_correct(response)

if ok:
    pass_test("response Content-Length matches body bytes")
else:
    fail_test("response Content-Length matches body bytes", message)


# =====================================================================
# EXTRA ROBUSTNESS - NOT SCORED AS SUBJECT FAILURES
# =====================================================================

section("EXTRA ROBUSTNESS - INFO ONLY")

# HEAD is not one of the methods mandated by this subject.
response = send_request(make_request("HEAD", "/"))
info_test(
    "HEAD is not mandatory in subject",
    get_first_line(response) or "no response"
)

# HTTP/1.0 is explicitly suggested as a reference point but not enforced.
response = send_request(
    b"GET / HTTP/1.0\r\n"
    b"Host: localhost\r\n"
    b"Connection: close\r\n\r\n"
)
info_test(
    "HTTP/1.0 is not enforced by subject",
    get_first_line(response) or "no response"
)

# Virtual host is explicitly out of scope.
info_test(
    "virtual-host routing",
    "not tested: subject says virtual host is out of scope"
)

# RFC edge cases: useful for robustness, but this project deliberately
# implements only a subset of HTTP.
extra_cases = [
    (
        "LF-only request",
        b"GET / HTTP/1.1\nHost: localhost\n\n"
    ),
    (
        "bare-CR request",
        b"GET / HTTP/1.1\rHost: localhost\r\r"
    ),
    (
        "very long URI",
        (
            "GET /" + ("a" * 5000) + " HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Connection: close\r\n\r\n"
        ).encode()
    ),
    (
        "unknown method BANANA",
        b"BANANA / HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"Connection: close\r\n\r\n"
    ),
]

for name, request in extra_cases:
    response = send_request(request)
    info_test(name, get_first_line(response) or "no response / timeout")
    health_check("alive after " + name)


# Chunked is especially relevant to CGI according to the subject.
# Since CGI is excluded from this tester, we only verify malformed chunked
# input cannot kill the server; exact response is informational here.
chunked_cases = [
    (
        "valid chunked POST (CGI excluded)",
        b"POST /upload HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"Transfer-Encoding: chunked\r\n"
        b"Connection: close\r\n\r\n"
        b"5\r\nhello\r\n"
        b"0\r\n\r\n"
    ),
    (
        "negative-looking chunk size",
        b"POST /upload HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"Transfer-Encoding: chunked\r\n"
        b"Connection: close\r\n\r\n"
        b"-1\r\nhello\r\n"
        b"0\r\n\r\n"
    ),
]

for name, request in chunked_cases:
    response = send_request(request)
    info_test(name, get_first_line(response) or "no response / timeout")
    health_check("alive after " + name)


# =====================================================================
# CLEANUP
# =====================================================================

section("CLEANUP")

cleanup_files = [
    marker_path,
    mapped_path,
    upload_path,
    auto_marker_path,
    noindex_marker_path,
]

for path in cleanup_files:
    try:
        if path and os.path.isfile(path):
            os.remove(path)
            print(YELLOW("[CLEAN]"), path)
    except OSError:
        pass

for directory in [
    auto_dir_path,
    noindex_dir_path,
]:
    try:
        if directory and os.path.isdir(directory):
            os.rmdir(directory)
            print(YELLOW("[CLEAN]"), directory)
    except OSError:
        pass


# =====================================================================
# FINAL
# =====================================================================

section("FINAL")

health_check("FINAL server still alive")

print()
print(BOLD("=" * 72))
print(BOLD("SUBJECT TEST SUMMARY"))
print(BOLD("=" * 72))

print(GREEN("PASS : %d" % passed))
print(RED("FAIL : %d" % failed))
print(YELLOW("INFO : %d" % info_count))
print(YELLOW("SKIP : %d" % skipped))

print()

if failed == 0 and skipped == 0:
    print(GREEN(BOLD("ALL TESTED SUBJECT REQUIREMENTS PASSED")))
elif failed == 0:
    print(YELLOW(BOLD(
        "NO SUBJECT TEST FAILED, BUT SOME MANDATORY COVERAGE WAS SKIPPED"
    )))
else:
    print(RED(BOLD("SOME SUBJECT REQUIREMENTS FAILED")))

if DELETE_URL_PREFIX is None or DELETE_LOCAL_DIR is None:
    print()
    print(YELLOW(BOLD("IMPORTANT:")))
    print(YELLOW(
        "DELETE is mandatory in Webserv v24.1, but no DELETE-enabled "
        "route is configured in this tester."
    ))
    print(YELLOW(
        "Add a DELETE-enabled location to your config, then fill "
        "DELETE_URL_PREFIX and DELETE_LOCAL_DIR above."
    ))

print()
print(YELLOW(
    "INFO cases are extra RFC/robustness observations and do not count "
    "as subject failures."
))
