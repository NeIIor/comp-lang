"""One-shot TCP echo on 127.0.0.1:9999 for testing socket_client.exe.

Usage (two terminals):
  1) python example/socket_echo_server.py
  2) run socket_client.exe from build/ (needs sfasmlib.dll with Winsock — see SOCKET_CLIENT.txt)

The client sends "42\\n"; this server sends the same bytes back; the client prints 42.
"""
from __future__ import annotations

import socket
import sys

HOST = "127.0.0.1"
PORT = 9999


def main() -> None:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as srv:
        srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        srv.bind((HOST, PORT))
        srv.listen(1)
        print(f"listening on {HOST}:{PORT} — run socket_client exe now", flush=True)
        conn, addr = srv.accept()
        with conn:
            print(f"accepted {addr}", flush=True)
            data = conn.recv(256)
            if not data:
                print("no data", flush=True)
                sys.exit(1)
            conn.sendall(data)
            print(f"echoed {data!r}", flush=True)


if __name__ == "__main__":
    main()
