#!/usr/bin/env python3

print("Content-Type: text/plain")
print()

with open("data.txt", "r") as f:
    print(f.read())