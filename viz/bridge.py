#!/usr/bin/env python3
"""
WebSocket bridge for local development: connects live.html to 42sh_debug.

Usage:
    python3 viz/bridge.py [--port 8765] [--shell ./42sh]

Then open viz/live.html?ws=8765 in your browser.

Not needed for static deployment (container2wasm handles it).
"""
import argparse, asyncio, os, pty, signal, fcntl, termios

try:
    import websockets
except ImportError:
    print("Install websockets: pip install websockets")
    raise SystemExit(1)

async def shell_session(websocket):
    shell = os.environ.get("SHELL_PATH", "./42sh")
    master, slave = pty.openpty()

    env = os.environ.copy()
    env["TERM"] = "xterm-256color"
    env["SH42_VIZ_DIR"] = "/tmp/42sh_viz"
    os.makedirs("/tmp/42sh_viz", exist_ok=True)

    pid = os.fork()
    if pid == 0:
        try:
            os.close(master)
            os.setsid()
            fcntl.ioctl(slave, termios.TIOCSCTTY, 0)
            os.dup2(slave, 0)
            os.dup2(slave, 1)
            os.dup2(slave, 2)
            os.close(slave)
            os.execve(shell, [shell, "-i"], env)
        except BaseException as e:
            # If anything fails before exec, MUST exit so the child can't
            # accept new connections on the inherited listening socket.
            try:
                os.write(2, f"bridge child exec failed: {e}\n".encode())
            except Exception:
                pass
            os._exit(127)
        # Should never reach here, but just in case execve returned (it can't):
        os._exit(127)

    os.close(slave)
    loop = asyncio.get_event_loop()

    async def read_pty():
        while True:
            try:
                data = await loop.run_in_executor(None, lambda: os.read(master, 4096))
                if not data:
                    break
                await websocket.send(data)
            except OSError:
                break

    async def write_pty():
        try:
            async for msg in websocket:
                raw = msg.encode() if isinstance(msg, str) else msg
                os.write(master, raw)
        except websockets.exceptions.ConnectionClosed:
            pass

    try:
        await asyncio.gather(asyncio.create_task(read_pty()), asyncio.create_task(write_pty()))
    finally:
        try: os.kill(pid, signal.SIGTERM)
        except ProcessLookupError: pass
        try: os.close(master)
        except OSError: pass
        os.waitpid(pid, os.WNOHANG)

async def main(port, shell):
    os.environ["SHELL_PATH"] = os.path.abspath(shell)
    print(f"42sh bridge on ws://localhost:{port}")
    print(f"Shell: {os.path.abspath(shell)}")
    print(f"Open viz/live.html?ws={port}")
    async with websockets.serve(shell_session, "localhost", port):
        await asyncio.Future()

if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("--port", type=int, default=8765)
    p.add_argument("--shell", default="./42sh")
    asyncio.run(main(**vars(p.parse_args())))
