#!/usr/bin/env python3
"""
Static file server that sends the cross-origin isolation headers required
by container2wasm (it needs SharedArrayBuffer, which requires COOP+COEP).

Usage: python3 viz/serve.py [--port 8080] [--root .]

Then open http://localhost:8080/viz/live.html
"""
import argparse
import http.server
import os
import socketserver


class COIHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        self.send_header("Cross-Origin-Resource-Policy", "cross-origin")
        super().end_headers()


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--port", type=int, default=8080)
    p.add_argument("--root", default=".")
    args = p.parse_args()

    os.chdir(args.root)
    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(("", args.port), COIHandler) as httpd:
        print(f"Serving {os.path.abspath(args.root)} on http://localhost:{args.port}")
        print(f"Open http://localhost:{args.port}/viz/live.html")
        httpd.serve_forever()


if __name__ == "__main__":
    main()
