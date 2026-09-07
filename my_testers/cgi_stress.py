import http.client
from concurrent.futures import ThreadPoolExecutor, as_completed

WORKERS = 20
ROUNDS = 5
BODY_SIZE = 100000000

# 所有线程共享同一份不可变 bytes，减少测试程序自身的内存占用。
body = b"A" * BODY_SIZE


def worker(number):
    conn = http.client.HTTPConnection(
        "127.0.0.1",
        8080,
        timeout=180
    )

    try:
        for turn in range(1, ROUNDS + 1):
            conn.request(
                "POST",
                "/directory/youpi.bla",
                body=body,
                headers={
                    "Content-Type": "application/octet-stream"
                }
            )

            response = conn.getresponse()
            status = response.status
            received = 0

            # 分块读取响应，不在 tester 中额外保存 100MB response。
            while True:
                chunk = response.read(65536)
                if not chunk:
                    break
                received += len(chunk)

            print(
                "worker=%d round=%d status=%d body=%d bytes"
                % (number, turn, status, received),
                flush=True
            )

            if status != 200:
                print(
                    "worker=%d FAILED: expected status 200"
                    % number,
                    flush=True
                )
                return False

            if received != BODY_SIZE:
                print(
                    "worker=%d FAILED: expected %d bytes, received %d"
                    % (number, BODY_SIZE, received),
                    flush=True
                )
                return False

        return True

    except Exception as error:
        print(
            "worker=%d ERROR: %s"
            % (number, error),
            flush=True
        )
        return False

    finally:
        conn.close()


with ThreadPoolExecutor(max_workers=WORKERS) as pool:
    tasks = [
        pool.submit(worker, number)
        for number in range(1, WORKERS + 1)
    ]

    results = [
        task.result()
        for task in as_completed(tasks)
    ]

success_count = sum(1 for result in results if result)

print(
    "成功 worker：%d/%d"
    % (success_count, WORKERS)
)
