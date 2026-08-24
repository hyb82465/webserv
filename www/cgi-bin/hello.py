#!/usr/bin/env python3

import os

print("Content-Type: text/html")
print()

print("<!DOCTYPE html>")
print("<html>")
print("<head>")
print("<title>CGI GET Test</title>")
print("</head>")
print("<body>")

print("<h1>CGI GET Test</h1>")

print("<p>REQUEST_METHOD: "
      + os.environ.get("REQUEST_METHOD", "")
      + "</p>")

print("<p>QUERY_STRING: "
      + os.environ.get("QUERY_STRING", "")
      + "</p>")

print("<p>SCRIPT_NAME: "
      + os.environ.get("SCRIPT_NAME", "")
      + "</p>")

print("<p>SCRIPT_FILENAME: "
      + os.environ.get("SCRIPT_FILENAME", "")
      + "</p>")

print("</body>")
print("</html>")