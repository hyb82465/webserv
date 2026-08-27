#!/bin/bash

HOST="http://127.0.0.1:8080"

echo "===== GET / ====="
curl -i "$HOST/"
echo
echo

echo "===== POST / should be 405 ====="
curl -i -X POST "$HOST/"
echo
echo

echo "===== GET missing file should be 404 ====="
curl -i "$HOST/does_not_exist"
echo
echo

echo "===== GET directory ====="
curl -i "$HOST/directory"
echo
echo

echo "===== GET directory with slash ====="
curl -i "$HOST/directory/"
echo
echo

echo "===== GET nested file ====="
curl -i "$HOST/directory/nop/other.pouic"
echo
echo

echo "===== POST normal Content-Length body ====="
curl -i \
    -X POST \
    -H "Content-Type: text/plain" \
    --data "Hello Webserv" \
    "$HOST/upload/test.txt"
echo
echo

echo "===== Check uploaded file ====="
cat uploads/test.txt 2>/dev/null
echo
echo

echo "===== DELETE uploaded file ====="
curl -i \
    -X DELETE \
    "$HOST/upload/test.txt"
echo
echo

echo "===== GET deleted file should be 404 ====="
curl -i "$HOST/upload/test.txt"
echo
echo

echo "===== Invalid HTTP version ====="
printf 'GET / HTTP/9.9\r\nHost: localhost\r\n\r\n' \
    | nc 127.0.0.1 8080
echo
echo

echo "===== Missing Host ====="
printf 'GET / HTTP/1.1\r\n\r\n' \
    | nc 127.0.0.1 8080
echo
echo

echo "===== Duplicate Host ====="
printf 'GET / HTTP/1.1\r\nHost: localhost\r\nHost: test\r\n\r\n' \
    | nc 127.0.0.1 8080
echo
echo

echo "===== Content-Length + Transfer-Encoding ====="
printf 'POST /upload/test.txt HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5\r\nTransfer-Encoding: chunked\r\n\r\nHello' \
    | nc 127.0.0.1 8080
echo
echo

echo "===== Small chunked body ====="
printf 'POST /upload/chunk.txt HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nHello\r\n6\r\n World\r\n0\r\n\r\n' \
    | nc 127.0.0.1 8080
echo
echo

echo "===== Done ====="
