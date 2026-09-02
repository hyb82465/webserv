#!/usr/bin/env python3

import os
import sys

length = int(os.environ.get("CONTENT_LENGTH", "0"))

body = sys.stdin.read(length)

print("Content-Type: text/plain")
print()

print("CGI POST TEST")
print("--------------------")
print("REQUEST_METHOD:",
      os.environ.get("REQUEST_METHOD", ""))

print("CONTENT_LENGTH:",
      os.environ.get("CONTENT_LENGTH", ""))

print("CONTENT_TYPE:",
      os.environ.get("CONTENT_TYPE", ""))

print("BODY:")
print(body)