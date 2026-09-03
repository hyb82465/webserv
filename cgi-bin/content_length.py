#!/usr/bin/env python3

import sys

body = "Hello CGI"

sys.stdout.write("Content-Type: text/plain\r\n")
sys.stdout.write(
    "Content-Length: %d\r\n" % len(body)
)
sys.stdout.write("\r\n")
sys.stdout.write(body)