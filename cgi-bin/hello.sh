#!/bin/sh

printf 'Content-Type: text/plain\r\n'
printf '\r\n'
printf 'Hello from shell CGI\n'
printf 'REQUEST_METHOD=%s\n' "$REQUEST_METHOD"
printf 'QUERY_STRING=%s\n' "$QUERY_STRING"