#!/usr/bin/env python3

import argparse
import http.client
import sys
from concurrent.futures import ThreadPoolExecutor, as_completed


def parse_args():
    parser = argparse.ArgumentParser(
        description="Concurrent CGI upload/echo stress test"
    )
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8080)
    parser.add_argument("--workers", type=int, default=20)
    parser.add_argument("--rounds", type=int, default=5)
    parser.add_argument("--body-size", type=int, default=100000000)
    parser.add_argument("--timeout", type=int, default=180)
    return parser.parse_args()


def worker(number, args, body):
    connection = http.client.HTTPConnection(
        args.host,
        args.port,
        timeout=args.timeout
    )
    try:
        for turn in range(1, args.rounds + 1):
            connection.request(
                "POST",
                "/directory/youpi.bla",
                body=body,
                headers={"Content-Type": "application/octet-stream"}
            )
            response = connection.getresponse()
            received = 0
            while True:
                chunk = response.read(65536)
                if not chunk:
                    break
                received += len(chunk)

            if response.status != 200:
                return False, (
                    "worker=%d round=%d expected status 200, got %d"
                    % (number, turn, response.status)
                )
            if received != args.body_size:
                return False, (
                    "worker=%d round=%d expected %d bytes, got %d"
                    % (number, turn, args.body_size, received)
                )
        return True, ""
    except Exception as error:
        return False, "worker=%d: %s" % (number, error)
    finally:
        connection.close()


def main():
    args = parse_args()
    if args.workers <= 0 or args.rounds <= 0 or args.body_size < 0:
        print("[FAIL] workers and rounds must be positive; body-size cannot be negative")
        return 1

    body = b"A" * args.body_size
    print(
        "Running CGI stress: %d workers x %d rounds x %d bytes"
        % (args.workers, args.rounds, args.body_size),
        flush=True
    )

    failures = []
    with ThreadPoolExecutor(max_workers=args.workers) as pool:
        tasks = [
            pool.submit(worker, number, args, body)
            for number in range(1, args.workers + 1)
        ]
        for task in as_completed(tasks):
            success, message = task.result()
            if not success:
                failures.append(message)

    if failures:
        print("[FAIL] CGI stress test: %d/%d workers failed"
              % (len(failures), args.workers))
        for message in failures:
            print("       " + message)
        return 1

    print("[PASS] CGI stress test: all %d workers completed %d rounds"
          % (args.workers, args.rounds))
    return 0


if __name__ == "__main__":
    sys.exit(main())
