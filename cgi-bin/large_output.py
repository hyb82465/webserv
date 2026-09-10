#!/usr/bin/env python3

import sys

output = sys.stdout.buffer

output.write(b"Content-Type: application/octet-stream\r\n")
output.write(b"\r\n")

chunk = b"A" * 65536
total = 140 * 1024 * 1024
written = 0

while written < total:
    remaining = total - written

    if remaining < len(chunk):
        output.write(chunk[:remaining])
        written += remaining
    else:
        output.write(chunk)
        written += len(chunk)
