#!/usr/bin/env python3

print("Content-Type: text/plain")
print()

print("CGI RELATIVE PATH TEST")
print("----------------------")

try:
    with open("data.txt", "r") as f:
        print(f.read())
except Exception as e:
    print("FAILED")
    print(str(e))