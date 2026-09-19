*This project has been created as part of the 42 curriculum by yihe, zhma.*

# webserv

## Description

`webserv` is a non-blocking HTTP server written in C++98. The goal of the
project is to understand how a web server accepts TCP connections, parses HTTP
requests, selects route configuration, serves resources, executes CGI programs,
and builds valid HTTP responses without relying on an existing web-server
library.

The server uses a single `poll()`-based event loop for listening sockets,
clients, and CGI pipes. Socket and pipe I/O is non-blocking. Regular files are
handled separately because they do not require readiness notification.

The implementation is intentionally limited to the subset of HTTP required by
the Webserv subject. It is not intended to replace a production server such as
NGINX.

## Features

- Multiple listening interface and port pairs.
- Non-blocking client and CGI I/O through one `poll()` event loop.
- HTTP request-line, header, `Content-Length`, and chunked-body parsing.
- `GET`, `POST`, and `DELETE` methods.
- Static file serving and index files.
- Directory listing with escaped HTML and URL-encoded links.
- Custom error pages and built-in fallback error pages.
- Per-server and per-location request-body limits.
- HTTP redirects.
- Multipart file uploads and configurable upload directories.
- Keep-alive connections and multiple requests on one connection.
- Client inactivity and CGI execution timeouts.
- CGI environment variables, request bodies, query strings, and response
  status/header processing.
- Multiple CGI types selected by file extension, including Python and shell
  examples.
- Cookie and file-backed session examples, with inactive session cleanup.

Virtual hosts based on the HTTP `Host` header are not implemented. Different
server configurations can instead listen on different interface/port pairs, as
required by the mandatory part of the subject.

## Architecture

The main components are:

- `ConfigParser` and `ConfigValidator`: tokenize, parse, and validate the
  configuration before sockets are created.
- `Server`: owns the central event loop, listening sockets, connected clients,
  and active CGI jobs.
- `Client` and `RequestState`: preserve connection buffers, parser progress,
  write progress, keep-alive state, and activity time.
- `RequestParser`: incrementally parses requests that can arrive over multiple
  reads.
- `RequestHandler`: performs location matching and handles static files,
  uploads, deletion, autoindex pages, redirects, and error responses.
- `CgiHandler`: manages one CGI child process, its environment, non-blocking
  stdin/stdout pipes, timeout, exit status, and bounded output buffer.
- `HttpRequest` and `HttpResponse`: represent parsed requests and generated
  responses.

A simplified request flow is:

```text
poll event
  -> accept or read client data
  -> incrementally parse request
  -> select the longest matching location
  -> apply method and route checks
  -> serve a resource or start CGI
  -> queue the HTTP response
  -> send when poll reports the client writable
```

## Instructions

### Requirements

The project is intended to run and be evaluated on Fedora Linux.

- A Linux environment.
- A C++ compiler with C++98 support.
- GNU Make.
- Python 3 for the provided Python CGI examples.
- `/bin/sh` for the shell CGI example.
- `curl` is optional but useful for manual testing.

No external C++ libraries or Boost are used.

If these tools are missing on a personal Fedora installation, they can usually
be installed with:

```bash
sudo dnf install gcc-c++ make python3 curl
```

The 42 evaluation environment normally already provides the required compiler
and build tools.

### Compilation on Fedora

From the repository root:

```bash
make
```

This creates the `webserv` executable. Object and dependency files are stored
under `.build/`.

Other Makefile rules:

```bash
make clean       # remove object and dependency files
make fclean      # remove build files and the executable
make re          # rebuild everything
make DEBUG=1     # enable debug logging
```

If the project was already built with debug logging disabled, use
`make re DEBUG=1` to force a rebuild.

### Execution

The program requires one configuration-file argument:

```bash
./webserv config/default.conf
```

Configuration paths are resolved relative to the directory from which
`webserv` is launched. Run the executable from the repository root when using
the provided configurations.

Stop the server with `Ctrl-C`.

The default demonstration configuration listens on:

```text
127.0.0.1:8080
127.0.0.1:8081
127.0.0.2:8080
127.0.0.3:8081
```

### Optional Docker development environment

Docker is not required to build, run, or evaluate the project. Development and
stress testing were also performed in an Ubuntu Docker container on macOS. The
same `make` and `./webserv config/default.conf` commands were used inside that
container. The authoritative submission workflow is the native Fedora workflow
described above.

## Configuration

The syntax is inspired by the NGINX `server` and `location` blocks. Statements
end with a semicolon. Body sizes are decimal byte counts.

A minimal example is:

```nginx
server {
    listen 127.0.0.1:8080;

    root ./www;
    index index.html;
    client_max_body_size 1000000;

    error_page 404 ./www/errors/404.html;

    location / {
        methods GET;
    }

    location /upload {
        methods POST;
        upload_store ./uploads;
    }

    location /files {
        root ./uploads;
        methods GET;
        autoindex on;
    }

    location /old {
        return 301 /new.html;
    }

    location /cgi-bin {
        root ./cgi-bin;
        methods GET POST;
        cgi .py /usr/bin/python3;
        cgi .sh /bin/sh;
    }
}
```

Supported server directives:

- `listen <interface>:<port>;`
- `root <path>;`
- `index <filename>;`
- `client_max_body_size <bytes>;`
- `error_page <status> <path>;`
- `location <URL-prefix> { ... }`

Supported location directives:

- `methods GET POST DELETE;`
- `root <path>;`
- `index <filename>;`
- `client_max_body_size <bytes>;`
- `autoindex on|off;`
- `upload_store <directory>;`
- `return <status> <URL>;`
- `cgi <extension> <executable>;`

Location matching uses the longest matching URL prefix. A location-level value
overrides the corresponding server-level default. For a location root, the
matched location prefix is removed before the remaining request path is
appended to the root.

CGI interpreter paths should preferably be absolute. CGI scripts are executed
with their containing directory as the working directory so that relative file
access inside the script behaves consistently.

## Usage examples

Start the server before running these commands.

Static page:

```bash
curl -i http://127.0.0.1:8080/
```

Static gallery route:

```bash
curl -i http://127.0.0.1:8080/images/
```

Multipart upload:

```bash
curl -i -F "file=@www/test.txt" http://127.0.0.1:8080/upload
```

Retrieve an uploaded file:

```bash
curl -i http://127.0.0.1:8080/uploads/test.txt
```

Delete a resource allowed by the `/delete` location:

```bash
curl -i -X DELETE http://127.0.0.1:8080/delete/example.txt
```

Python and shell CGI examples:

```bash
curl -i "http://127.0.0.1:8080/cgi-bin/hello.py?type=python"
curl -i "http://127.0.0.1:8080/cgi-bin/hello.sh?type=shell"
```

Cookie example:

```bash
curl -i -c /tmp/webserv_cookies.txt \
  http://127.0.0.1:8080/cgi-bin/cookie.py

curl -i -b /tmp/webserv_cookies.txt \
  http://127.0.0.1:8080/cgi-bin/cookie.py
```

Session example:

```bash
curl -i -c /tmp/webserv_session.txt \
  http://127.0.0.1:8080/cgi-bin/session.py

curl -i -b /tmp/webserv_session.txt \
  http://127.0.0.1:8080/cgi-bin/session.py
```

The Session ID is stored in the Cookie, while the visit count is stored under
the non-public `session_data/` directory. Inactive Session files are removed
after 30 minutes when the Session CGI next performs cleanup. The directory is
excluded from Git.

## Testing

The supplied school tester can be run while the server is active:

```bash
./tester http://127.0.0.1:8080
```

Additional black-box and regression tests are available under `my_testers/`:

```bash
python3 my_testers/webserv_tester.py
python3 my_testers/client_timeout.py
python3 my_testers/cgi_stress.py
```

Run the tests from the repository root while the server is running with
`config/default.conf`. `webserv_tester.py` checks HTTP parsing, configured
routes, body-size limits, uploads, downloads, redirects, multiple listeners,
keep-alive, CGI behavior, and the Cookie and Session examples.

`client_timeout.py` takes approximately 35 seconds because it verifies that an
inactive connection is closed without making the server unavailable.

`cgi_stress.py` is intentionally resource intensive: by default it runs 20
workers, each sending five 100,000,000-byte CGI requests. Only run the default
test in an environment with enough memory and swap space. A smaller run is:

```bash
python3 my_testers/cgi_stress.py \
    --workers 2 \
    --rounds 2 \
    --body-size 1000000 \
    --timeout 30
```

Before submission, perform a clean build and full regression run:

```bash
make fclean
make
make
./webserv config/default.conf
```

The second `make` should report that there is nothing to rebuild. Then run the
school tester and the three custom testers from another terminal.

Some evaluation checks are intentionally manual: inspect the single `poll()`
event loop and every `read`/`recv`/`write`/`send` result, try conflicting listen
addresses with two server processes, modify a custom error page, and inspect
the site and HTTP headers in a browser.

### Siege stress test

With the server running, send 1,000 GET requests to an empty static page:

```bash
siege -b -c 20 -r 50 http://127.0.0.1:8080/directory/
```

In another terminal, watch the server's physical memory usage:

```bash
WEBSERV_PID=$(pgrep -n -x webserv)
watch -n 1 "ps -o pid=,rss=,etime=,cmd= -p $WEBSERV_PID"
```

Check that `Availability` is above 99.5% and `Failed transactions` is zero.
The `RSS` column shows physical memory in KiB. It may rise during the test, but
after repeated identical tests it should stabilize instead of growing without
limit. When using Docker, `docker stats <container-id>` can also be used to
watch the total container memory.

## Technical choices and limitations

- C++98 is used throughout the server.
- One `poll()` loop coordinates listening sockets, clients, and CGI pipes.
- CGI is executed with `fork()`, `pipe()`, `dup2()`, and `execve()`.
- CGI responses are buffered with a per-process limit of 128 MiB; they are not
  streamed directly to clients.
- CGI execution has a five-second inactivity timeout.
- Client connections have a 30-second inactivity timeout while actively
  monitored for input or output.
- Request and response behavior implements the project subset of HTTP rather
  than every feature from the HTTP specifications.
- HTTP `Host`-based virtual hosting is out of scope and not implemented.
- The provided configuration and CGI executable paths target a standard Linux
  environment and are compatible with the Fedora evaluation environment.

## Resources

Classic references used while learning and implementing the project:

- [RFC 9110 - HTTP Semantics](https://www.rfc-editor.org/rfc/rfc9110)
- [RFC 9112 - HTTP/1.1](https://www.rfc-editor.org/rfc/rfc9112)
- [RFC 3875 - The Common Gateway Interface (CGI) Version 1.1](https://www.rfc-editor.org/rfc/rfc3875)
- [Linux `poll(2)` manual](https://man7.org/linux/man-pages/man2/poll.2.html)
- [Linux `socket(2)` manual](https://man7.org/linux/man-pages/man2/socket.2.html)
- [Linux `execve(2)` manual](https://man7.org/linux/man-pages/man2/execve.2.html)
- [MDN HTTP documentation](https://developer.mozilla.org/en-US/docs/Web/HTTP)
- [NGINX documentation](https://nginx.org/en/docs/)
- [cppreference C++ language reference](https://en.cppreference.com/w/cpp)

### Use of AI

AI tools were used as learning and review aids during the project. They helped
with:

- explaining unfamiliar system concepts such as non-blocking descriptors,
  `poll()`, socket options, signals, pipes, CGI environment variables, and
  process exit behavior;
- reviewing the event loop, request parsing, CGI lifecycle, resource cleanup,
  configuration validation, HTTP header handling, and path-safety edge cases;
- suggesting focused black-box, malformed-request, timeout, upload, concurrent
  CGI, Cookie, and Session tests;
- diagnosing failures observed in the school tester and Docker environment;
- discussing architecture and refactoring trade-offs; and
- drafting and checking this README against the project requirements.

AI output was not accepted as authoritative. Suggestions were read, questioned,
adapted to the project's C++98 design, compiled with the required warning flags,
and verified with manual tests, custom tests, the school tester, and peer
discussion. The authors remain responsible for understanding and defending the
submitted implementation.
