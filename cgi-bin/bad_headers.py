#!/usr/bin/env python3

import sys

body = "Hello CGI"

sys.stdout.write("Content-Type: text/plain\r\n")
sys.stdout.write("Content-Length: 999\r\n")
sys.stdout.write("Connection: close\r\n")
sys.stdout.write("Transfer-Encoding: chunked\r\n")
sys.stdout.write("X-CGI-Test: kept\r\n")
sys.stdout.write("\r\n")
sys.stdout.write(body)
