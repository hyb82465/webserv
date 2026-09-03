#!/usr/bin/python3

import sys

body = "Created by CGI"

sys.stdout.write("Status: 201 Created\r\n")
sys.stdout.write("Content-Type: text/plain\r\n")
sys.stdout.write("\r\n")
sys.stdout.write(body)