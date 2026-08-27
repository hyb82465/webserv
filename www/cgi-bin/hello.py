#!/usr/bin/env python3

import sys
import os

body = sys.stdin.read()

print("Content-Type: text/plain")
print()

print("Method:", os.environ.get("REQUEST_METHOD"))
print("Content-Type:", os.environ.get("CONTENT_TYPE"))
print("Content-Length:", os.environ.get("CONTENT_LENGTH"))
print("Body:", body)