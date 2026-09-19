#!/usr/bin/env python3

import os
import socket
import sys
import time


DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 8080
TIMEOUT = 3

passed = 0
failed = 0


def record(name, success, detail=""):
    global passed, failed
    if success:
        passed += 1
        print("[PASS] " + name)
    else:
        failed += 1
        print("[FAIL] " + name)
        if detail:
            print("       " + detail)


def send_request(request, host=DEFAULT_HOST, port=DEFAULT_PORT,
                 timeout=TIMEOUT):
    if isinstance(request, str):
        request = request.encode()
    sock = socket.create_connection((host, port), timeout=timeout)
    sock.settimeout(timeout)
    response = b""
    try:
        if request:
            sock.sendall(request)
        while True:
            try:
                data = sock.recv(65536)
            except socket.timeout:
                break
            if not data:
                break
            response += data
    finally:
        sock.close()
    return response


def send_parts(parts, delay=0.05, timeout=TIMEOUT):
    sock = socket.create_connection((DEFAULT_HOST, DEFAULT_PORT), timeout=timeout)
    sock.settimeout(timeout)
    response = b""
    try:
        for part in parts:
            if isinstance(part, str):
                part = part.encode()
            sock.sendall(part)
            time.sleep(delay)
        while True:
            try:
                data = sock.recv(65536)
            except socket.timeout:
                break
            if not data:
                break
            response += data
    finally:
        sock.close()
    return response


def make_request(method, path, body=b"", headers=None, close=True):
    if isinstance(body, str):
        body = body.encode()
    fields = [method + " " + path + " HTTP/1.1", "Host: localhost"]
    if headers:
        fields.extend(headers)
    lower = [field.lower() for field in fields]
    if body and not any(field.startswith("content-length:") for field in lower) \
            and not any(field.startswith("transfer-encoding:") for field in lower):
        fields.append("Content-Length: %d" % len(body))
    if close:
        fields.append("Connection: close")
    return ("\r\n".join(fields) + "\r\n\r\n").encode() + body


def split_response(response):
    position = response.find(b"\r\n\r\n")
    if position == -1:
        return response, b""
    return response[:position], response[position + 4:]


def get_status(response):
    try:
        return int(response.split(b"\r\n", 1)[0].split()[1])
    except (IndexError, ValueError):
        return -1


def get_header(response, name):
    header, _ = split_response(response)
    wanted = name.lower()
    for line in header.split(b"\r\n")[1:]:
        if b":" not in line:
            continue
        key, value = line.split(b":", 1)
        if key.decode("iso-8859-1").strip().lower() == wanted:
            return value.decode("iso-8859-1").strip()
    return None


def get_body(response):
    return split_response(response)[1]


def read_file(path):
    try:
        with open(path, "rb") as file:
            return file.read()
    except OSError:
        return None


def get_cookie(response, name):
    header = get_header(response, "Set-Cookie")
    if header is None:
        return ""
    cookie = header.split(";", 1)[0]
    key, separator, value = cookie.partition("=")
    if separator and key.strip() == name:
        return value.strip()
    return ""


def first_line(response):
    if not response:
        return "no response"
    return response.split(b"\r\n", 1)[0].decode("iso-8859-1", errors="replace")


def expect_status(name, request, expected, host=DEFAULT_HOST,
                  port=DEFAULT_PORT, timeout=TIMEOUT):
    try:
        response = send_request(request, host, port, timeout)
        actual = get_status(response)
        record(name, actual == expected,
               "expected %d, got %d (%s)" % (expected, actual, first_line(response)))
        return response
    except OSError as error:
        record(name, False, "socket error: %s" % error)
        return b""


def expect_content_length(name, response):
    value = get_header(response, "Content-Length")
    try:
        declared = int(value) if value is not None else -1
    except ValueError:
        declared = -1
    actual = len(get_body(response))
    record(name, declared == actual,
           "Content-Length=%r, body bytes=%d" % (value, actual))


def redirect_test(path, expected_location):
    response = expect_status("GET %s redirects" % path,
                             make_request("GET", path), 301)
    actual = get_header(response, "Location")
    record("GET %s Location" % path, actual == expected_location,
           "expected %r, got %r" % (expected_location, actual))


def multipart_request(filename, content, boundary="----WebservBoundary"):
    body = (
        ("--" + boundary + "\r\n").encode()
        + ('Content-Disposition: form-data; name="file"; filename="%s"\r\n'
           % filename).encode()
        + b"Content-Type: application/octet-stream\r\n\r\n"
        + content
        + b"\r\n"
        + ("--" + boundary + "--\r\n").encode()
    )
    return make_request(
        "POST", "/upload", body,
        ["Content-Type: multipart/form-data; boundary=" + boundary,
         "Content-Length: %d" % len(body)]
    )


def read_one_response(sock, pending=b""):
    while b"\r\n\r\n" not in pending:
        pending += sock.recv(4096)
    header_end = pending.find(b"\r\n\r\n")
    header = pending[:header_end]
    length = None
    for line in header.split(b"\r\n")[1:]:
        if line.lower().startswith(b"content-length:"):
            length = int(line.split(b":", 1)[1].strip())
            break
    if length is None:
        raise ValueError("response has no Content-Length")
    total = header_end + 4 + length
    while len(pending) < total:
        pending += sock.recv(4096)
    return pending[:total], pending[total:]


def test_keep_alive():
    name = "two sequential requests use one keep-alive connection"
    sock = None
    try:
        sock = socket.create_connection((DEFAULT_HOST, DEFAULT_PORT), timeout=3)
        sock.settimeout(3)
        sock.sendall(make_request("GET", "/", close=False))
        first, pending = read_one_response(sock)
        sock.sendall(make_request("GET", "/", close=True))
        second, pending = read_one_response(sock, pending)
        record(name, get_status(first) == 200 and get_status(second) == 200,
               "statuses: %d, %d" % (get_status(first), get_status(second)))
    except (OSError, ValueError) as error:
        record(name, False, str(error))
    finally:
        if sock is not None:
            sock.close()


def test_client_disconnect():
    name = "incomplete client disconnect does not kill server"
    try:
        sock = socket.create_connection((DEFAULT_HOST, DEFAULT_PORT), timeout=3)
        sock.sendall(b"POST /upload HTTP/1.1\r\nHost: localhost\r\nContent-Length: 100\r\n\r\nhello")
        sock.close()
        time.sleep(0.1)
        response = send_request(make_request("GET", "/"))
        record(name, get_status(response) == 200,
               "health check returned " + first_line(response))
    except OSError as error:
        record(name, False, str(error))


def test_delete():
    name = "DELETE removes a file from configured directory"
    local_path = "./www/delete/__webserv_delete_test__.txt"
    try:
        with open(local_path, "wb") as file:
            file.write(b"delete me\n")
        response = send_request(
            make_request("DELETE", "/delete/__webserv_delete_test__.txt")
        )
        record(name, get_status(response) == 200 and not os.path.exists(local_path),
               "%s; file exists=%s" % (first_line(response), os.path.exists(local_path)))
    except (OSError, IOError) as error:
        record(name, False, str(error))
    finally:
        if os.path.isfile(local_path):
            os.remove(local_path)


def test_location_body_limit():
    path = "./www/post_body"
    original = read_file(path)
    if original is None:
        record("location body-size test setup", False, "cannot read " + path)
        return
    try:
        expect_status("location accepts body equal to 100 bytes",
                      make_request("POST", "/post_body", b"A" * 100), 200)
        expect_status("location rejects body above 100 bytes",
                      make_request("POST", "/post_body", b"A" * 101), 413)
    finally:
        with open(path, "wb") as file:
            file.write(original)


def test_cgi_examples():
    response = expect_status(
        "Python CGI GET",
        make_request("GET", "/cgi-bin/hello.py?language=python"),
        200,
    )
    body = get_body(response)
    record("Python CGI receives GET environment and query",
           b"REQUEST_METHOD=GET" in body
           and b"QUERY_STRING=language=python" in body,
           "body=%r" % body)

    response = expect_status(
        "Python CGI POST",
        make_request("POST", "/cgi-bin/post.py", b"hello-cgi",
                     ["Content-Type: text/plain"]),
        200,
    )
    body = get_body(response)
    record("Python CGI receives POST body and headers",
           b"REQUEST_METHOD: POST" in body
           and b"CONTENT_TYPE: text/plain" in body
           and b"hello-cgi" in body,
           "body=%r" % body)

    response = expect_status("CGI relative file access",
                             make_request("GET", "/cgi-bin/relative.py"), 200)
    record("CGI runs in its script directory",
           b"Hello from relative file!" in get_body(response),
           "body=%r" % get_body(response))

    response = expect_status("shell CGI",
                             make_request("GET", "/cgi-bin/hello.sh?type=sh"), 200)
    body = get_body(response)
    record("second CGI system receives environment",
           b"Hello from shell CGI" in body
           and b"REQUEST_METHOD=GET" in body
           and b"QUERY_STRING=type=sh" in body,
           "body=%r" % body)


def test_cookie_and_session():
    first = expect_status("Cookie CGI creates a cookie",
                          make_request("GET", "/cgi-bin/cookie.py"), 200)
    cookie = get_cookie(first, "webserv_bonus")
    record("Cookie CGI sends webserv_bonus", cookie == "hello",
           "cookie=%r" % cookie)
    second = expect_status(
        "Cookie CGI accepts the cookie",
        make_request("GET", "/cgi-bin/cookie.py",
                     headers=["Cookie: webserv_bonus=" + cookie]),
        200,
    )
    record("Cookie value reaches CGI",
           b"Cookie received: hello" in get_body(second),
           "body=%r" % get_body(second))

    first = expect_status("Session CGI creates a session",
                          make_request("GET", "/cgi-bin/session.py"), 200)
    session_id = get_cookie(first, "webserv_session")
    session_path = "./session_data/" + session_id
    try:
        record("Session CGI sends a valid session ID",
               len(session_id) == 32
               and all(character in "0123456789abcdef"
                       for character in session_id),
               "session_id=%r" % session_id)
        second = expect_status(
            "Session CGI reuses the session",
            make_request("GET", "/cgi-bin/session.py",
                         headers=["Cookie: webserv_session=" + session_id]),
            200,
        )
        record("Session visit count increases",
               b"Visit count: 1" in get_body(first)
               and b"Visit count: 2" in get_body(second),
               "first=%r, second=%r" % (get_body(first), get_body(second)))
    finally:
        if session_id and os.path.isfile(session_path):
            os.remove(session_path)


def run_tests():
    root_response = expect_status("GET /", make_request("GET", "/"), 200)
    expect_content_length("GET / Content-Length matches body", root_response)
    missing_response = expect_status("missing static file",
                                     make_request("GET", "/does_not_exist"), 404)
    record("configured 404 page is served",
           get_body(missing_response) == read_file("./www/errors/404.html"),
           "response body differs from ./www/errors/404.html")
    expect_status("POST / is not allowed", make_request("POST", "/"), 405)
    expect_status("DELETE / is not allowed", make_request("DELETE", "/"), 405)
    expect_status("HEAD / is not implemented", make_request("HEAD", "/"), 405)
    expect_status("HTTP/2.0 is rejected",
                  b"GET / HTTP/2.0\r\nHost: localhost\r\nConnection: close\r\n\r\n", 505)
    expect_status("HTTP/1.0 is rejected",
                  b"GET / HTTP/1.0\r\nHost: localhost\r\nConnection: close\r\n\r\n", 505)

    redirect_test("/directory", "/directory/")
    directory_response = expect_status("GET /directory/",
                                       make_request("GET", "/directory/"), 200)
    record("directory serves configured index file",
           get_body(directory_response)
           == read_file("./YoupiBanane/youpi.bad_extension"),
           "response differs from configured index")
    redirect_test("/images", "/images/")
    images_response = expect_status("GET /images/",
                                    make_request("GET", "/images/"), 200)
    record("route serves content from its configured root",
           get_body(images_response) == read_file("./assets/gallery.html"),
           "response differs from ./assets/gallery.html")
    expect_status("POST /images/ is not allowed", make_request("POST", "/images/"), 405)
    redirect_test("/old", "/new.html")

    for host, port, limit in [("127.0.0.2", 8080, 5),
                              ("127.0.0.3", 8081, 100)]:
        expect_status("body above %d bytes on %s:%d" % (limit, host, port),
                      make_request("POST", "/", b"a" * (limit + 1)),
                      413, host, port)
        expect_status("body equal to %d bytes is accepted by size check" % limit,
                      make_request("POST", "/", b"a" * limit),
                      405, host, port)
        expect_status("body below %d bytes is accepted by size check" % limit,
                      make_request("POST", "/", b"a" * (limit - 1)),
                      405, host, port)

    test_location_body_limit()

    upload_name = "__webserv_upload_test__.bin"
    upload_path = "./uploads/" + upload_name
    upload_content = b"A\x00B\xffC\n"
    if os.path.isfile(upload_path):
        os.remove(upload_path)
    try:
        response = send_request(multipart_request(upload_name, upload_content))
        record("multipart upload status", get_status(response) in (200, 201),
               first_line(response))
        saved = b""
        if os.path.isfile(upload_path):
            with open(upload_path, "rb") as file:
                saved = file.read()
        record("multipart upload writes exact binary content",
               saved == upload_content,
               "file missing or content differs")
        downloaded = expect_status("uploaded file can be downloaded",
                                   make_request("GET", "/uploads/" + upload_name),
                                   200)
        record("downloaded file matches uploaded content",
               get_body(downloaded) == upload_content,
               "downloaded content differs")
    except (OSError, IOError) as error:
        record("multipart upload", False, str(error))
    finally:
        if os.path.isfile(upload_path):
            os.remove(upload_path)

    expect_status(
        "multipart without boundary",
        make_request("POST", "/upload", b"hello",
                     ["Content-Type: multipart/form-data", "Content-Length: 5"]),
        400
    )
    escape_path = "./__webserv_escape_test__.txt"
    if os.path.isfile(escape_path):
        os.remove(escape_path)
    response = expect_status(
        "unsafe multipart filename",
        multipart_request("../__webserv_escape_test__.txt", b"evil"),
        400
    )
    record("unsafe multipart filename creates no escaped file",
           not os.path.exists(escape_path), "unexpected file: " + escape_path)

    response = send_parts([
        "GET / HT", "TP/1.", "1\r\n", "Host: local", "host\r\n",
        "Connection: close\r\n", "\r\n"
    ])
    record("request line and headers survive TCP fragmentation",
           get_status(response) == 200, first_line(response))

    response = send_parts([
        "POST /directory/youpi.bla HTTP/1.1\r\n",
        "Host: localhost\r\nContent-Length: 5\r\nConnection: close\r\n\r\n",
        "he", "llo"
    ])
    record("POST body survives TCP fragmentation",
           get_status(response) == 200 and get_body(response) == b"HELLO",
           "%s, body=%r" % (first_line(response), get_body(response)))

    fixed_cases = [
        ("invalid request line", b"HELLO\r\nHost: localhost\r\nConnection: close\r\n\r\n", 400),
        ("missing HTTP version", b"GET /\r\nHost: localhost\r\nConnection: close\r\n\r\n", 400),
        ("unknown method", b"BANANA / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n", 405),
        ("missing Host", b"GET / HTTP/1.1\r\nConnection: close\r\n\r\n", 400),
        ("empty Host", b"GET / HTTP/1.1\r\nHost:\r\nConnection: close\r\n\r\n", 400),
        ("header without colon", b"GET / HTTP/1.1\r\nHost: localhost\r\nBrokenHeader\r\nConnection: close\r\n\r\n", 400),
        ("duplicate Host", b"GET / HTTP/1.1\r\nHost: localhost\r\nHost: example.com\r\nConnection: close\r\n\r\n", 400),
        ("invalid Content-Length", b"POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: abc\r\nConnection: close\r\n\r\n", 400),
        ("negative Content-Length", b"POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: -1\r\nConnection: close\r\n\r\n", 400),
        ("Content-Length with Transfer-Encoding", b"POST /directory/youpi.bla HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5\r\nTransfer-Encoding: chunked\r\nConnection: close\r\n\r\nhello", 400),
        ("malformed chunk size", b"POST /directory/youpi.bla HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\nConnection: close\r\n\r\nZ\r\nhello\r\n0\r\n\r\n", 400),
    ]
    for name, request, expected in fixed_cases:
        expect_status(name, request, expected)

    expect_status("mixed-case header names",
                  b"GET / HTTP/1.1\r\nhOsT: localhost\r\ncOnNeCtIoN: close\r\n\r\n", 200)
    expect_status("query string on static GET",
                  make_request("GET", "/?hello=world"), 200)
    expect_status("request line above 8192 bytes",
                  make_request("GET", "/" + "a" * 9000), 414)
    expect_status("headers above 32768 bytes",
                  make_request("GET", "/", headers=["X-Large: " + "a" * 33000]), 431)

    chunked = (
        b"POST /directory/youpi.bla HTTP/1.1\r\n"
        b"Host: localhost\r\nTransfer-Encoding: chunked\r\n"
        b"Connection: close\r\n\r\n"
        b"5\r\nHello\r\n6\r\n World\r\n0\r\n\r\n"
    )
    response = expect_status("valid chunked CGI POST", chunked, 200)
    record("chunked CGI body is decoded correctly",
           get_body(response) == b"HELLO WORLD",
           "body=%r" % get_body(response))

    test_cgi_examples()
    response = expect_status("CGI explicit Content-Length",
                             make_request("GET", "/cgi-bin/content_length.py"), 200)
    expect_content_length("CGI Content-Length matches body", response)
    expect_status("CGI Status header", make_request("GET", "/cgi-bin/status.py"), 201)
    expect_status("CGI non-zero exit", make_request("GET", "/cgi-bin/error.py"), 500)
    expect_status("CGI timeout", make_request("GET", "/cgi-bin/infinite.py"),
                  500, timeout=8)
    expect_status(
        "missing CGI script returns controlled error",
        make_request("GET", "/cgi-bin/__missing__.py"),
        500,
    )

    listeners = [
        ("127.0.0.1", 8080, b"Hello Webserv"),
        ("127.0.0.1", 8081, b"Hello Webserv"),
        ("127.0.0.2", 8080, b"THIS IS SERVER 8080"),
        ("127.0.0.3", 8081, b"THIS IS SERVER 8081"),
    ]
    for host, port, marker in listeners:
        response = expect_status("listener %s:%d" % (host, port),
                                 make_request("GET", "/"), 200, host, port)
        record("website content on %s:%d" % (host, port),
               marker in get_body(response),
               "expected body marker %r" % marker)

    test_keep_alive()
    test_client_disconnect()
    test_delete()
    test_cookie_and_session()


def main():
    try:
        probe = send_request(make_request("GET", "/"), timeout=1)
    except OSError as error:
        print("[FAIL] cannot connect to webserv at %s:%d: %s"
              % (DEFAULT_HOST, DEFAULT_PORT, error))
        print("       start it with: ./webserv config/default.conf")
        return 1
    if get_status(probe) != 200:
        print("[FAIL] webserv preflight returned " + first_line(probe))
        print("       make sure it is running with config/default.conf")
        return 1
    try:
        run_tests()
    except KeyboardInterrupt:
        print("\n[FAIL] tester interrupted")
        return 1
    print("\n========================================")
    print("PASS: %d" % passed)
    print("FAIL: %d" % failed)
    print("TOTAL: %d" % (passed + failed))
    print("========================================")
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
