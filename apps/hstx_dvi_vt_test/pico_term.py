#!/usr/bin/env python3
# Run a program with its screen on the Pico terminal and its keyboard here.
#   ./pico_term.py [-p /dev/ttyACM0] [-- program args...]   (default: your shell)
# Output goes to the Pico; this keyboard and the Pico's replies go to the program.
# Ctrl-] quits.
import fcntl, os, pty, re, select, struct, sys, termios, time, tty

args = sys.argv[1:]
port = "/dev/ttyACM0"
if args[:1] == ["-p"]:
    port, args = args[1], args[2:]
if args[:1] == ["--"]:
    args = args[1:]
cmd = args or [os.environ.get("SHELL", "/bin/sh")]

ser = os.open(port, os.O_RDWR | os.O_NOCTTY)
tty.setraw(ser)  # No echo (it would bounce replies back to the screen) and no CRLF translation

# Ask the Pico for its size: ESC [ 18 t -> ESC [ 8 ; rows ; cols t
termios.tcflush(ser, termios.TCIFLUSH)
os.write(ser, b"\x1b[18t")
reply, deadline = b"", time.time() + 1
while time.time() < deadline and not re.search(rb"\x1b\[8;\d+;\d+t", reply):
    if select.select([ser], [], [], 0.1)[0]:
        reply += os.read(ser, 64)
m = re.search(rb"\x1b\[8;(\d+);(\d+)t", reply)
rows, cols = (int(m[1]), int(m[2])) if m else (60, 80)
print(f"Pico terminal is {cols}x{rows}" + ("" if m else " (no reply, guessed)") + ". Ctrl-] quits.")

pid, master = pty.fork()
if pid == 0:
    os.environ["TERM"] = os.environ.get("PICO_TERM", "xterm-256color")
    os.execvp(cmd[0], cmd)

fcntl.ioctl(master, termios.TIOCSWINSZ, struct.pack("HHHH", rows, cols, 0, 0))

def write_all(fd, data):
    while data:
        data = data[os.write(fd, data):]

stdin = sys.stdin.fileno()
saved = termios.tcgetattr(stdin)
tty.setraw(stdin)
try:
    while True:
        for fd in select.select([stdin, ser, master], [], [])[0]:
            try:
                data = os.read(fd, 1024)
            except OSError:  # The program exited
                data = b""
            if fd == master:
                if not data:
                    raise SystemExit
                write_all(ser, data)
            elif fd == stdin:
                if b"\x1d" in data:  # Ctrl-]
                    raise SystemExit
                write_all(master, data)
            else:  # Replies from the Pico
                write_all(master, data)
finally:
    termios.tcsetattr(stdin, termios.TCSADRAIN, saved)
    os.close(ser)
