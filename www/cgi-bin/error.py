#!/usr/bin/env python3

print("Content-Type: text/plain")
print()

print("CGI ERROR TEST")
print("The following error is intentional.")

raise RuntimeError("Intentional CGI failure")