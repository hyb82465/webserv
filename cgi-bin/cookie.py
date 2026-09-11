#!/usr/bin/env python3

import os
import sys
from http.cookies import SimpleCookie, CookieError

cookie_header = os.environ.get("HTTP_COOKIE", "")

cookies = SimpleCookie()

try:
    cookies.load(cookie_header)
except CookieError:
    cookies = SimpleCookie()

cookie_name = "webserv_bonus"
cookie_value = ""

if cookie_name in cookies:
    cookie_value = cookies[cookie_name].value

sys.stdout.write("Content-Type: text/plain\r\n")

if not cookie_value:
    sys.stdout.write(
        "Set-Cookie: webserv_bonus=hello; "
        "Path=/; HttpOnly; SameSite=Lax\r\n"
    )

sys.stdout.write("\r\n")

if cookie_value:
    sys.stdout.write("Cookie received: %s\n" % cookie_value)
else:
    sys.stdout.write("No cookie received. A cookie has been created.\n")
