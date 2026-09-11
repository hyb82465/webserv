#!/usr/bin/env python3

import os
import secrets
import sys
import time
from http.cookies import SimpleCookie, CookieError


COOKIE_NAME = "webserv_session"
SESSION_ID_LENGTH = 32
SESSION_LIFETIME = 30 * 60


def is_valid_session_id(value):
    if len(value) != SESSION_ID_LENGTH:
        return False

    allowed = "0123456789abcdef"

    for character in value:
        if character not in allowed:
            return False

    return True


script_path = os.path.abspath(__file__)
cgi_directory = os.path.dirname(script_path)
project_directory = os.path.dirname(cgi_directory)
session_directory = os.path.join(
    project_directory,
    "session_data"
)

os.makedirs(session_directory, exist_ok=True)

current_time = time.time()

for filename in os.listdir(session_directory):
    if not is_valid_session_id(filename):
        continue

    path = os.path.join(session_directory, filename)

    try:
        if os.path.islink(path):
            continue

        last_activity = os.path.getmtime(path)

        if current_time - last_activity > SESSION_LIFETIME:
            os.remove(path)
    except OSError:
        pass

cookie_header = os.environ.get("HTTP_COOKIE", "")
cookies = SimpleCookie()

try:
    cookies.load(cookie_header)
except CookieError:
    cookies = SimpleCookie()


session_id = ""

if COOKIE_NAME in cookies:
    candidate = cookies[COOKIE_NAME].value

    if is_valid_session_id(candidate):
        session_id = candidate


new_session = False
session_file = ""

if session_id:
    session_file = os.path.join(
        session_directory,
        session_id
    )

    if not os.path.isfile(session_file):
        session_id = ""


if not session_id:
    new_session = True

    while True:
        session_id = secrets.token_hex(16)
        session_file = os.path.join(
            session_directory,
            session_id
        )

        if not os.path.exists(session_file):
            break


visits = 0

if os.path.isfile(session_file):
    try:
        with open(session_file, "r") as file:
            content = file.read().strip()
            visits = int(content)
    except (OSError, ValueError):
        visits = 0


visits += 1

with open(session_file, "w") as file:
    file.write(str(visits))


body = (
    "Session ID: %s\n"
    "Visit count: %d\n"
    % (session_id, visits)
)


sys.stdout.write("Content-Type: text/plain\r\n")

if new_session:
    sys.stdout.write(
        "Set-Cookie: %s=%s; "
        "Path=/; HttpOnly; SameSite=Lax\r\n"
        % (COOKIE_NAME, session_id)
    )

sys.stdout.write("\r\n")
sys.stdout.write(body)
