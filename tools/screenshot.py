"""抓取 Anvil 窗口画面（绕开 PrintWindow 对 DX 交换链无效的问题，直接 BitBlt 屏幕）。"""

import ctypes
import struct
import sys
import time
import zlib
from ctypes import wintypes

u = ctypes.WinDLL("user32", use_last_error=True)
g = ctypes.WinDLL("gdi32", use_last_error=True)

# 本脚本必须自己声明 DPI 感知，否则 GetWindowRect / BitBlt 用的是被 Windows
# 虚拟化过的坐标（例如 175% 缩放下屏幕会被当成 1463x823），抓出来的位置全错。
try:
    u.SetProcessDpiAwarenessContext(ctypes.c_void_p(-4))  # PER_MONITOR_AWARE_V2
except (AttributeError, OSError):
    try:
        ctypes.WinDLL("shcore").SetProcessDpiAwareness(2)
    except (AttributeError, OSError):
        u.SetProcessDPIAware()

WNDENUMPROC = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)


def _text(hwnd):
    n = u.GetWindowTextLengthW(hwnd)
    buf = ctypes.create_unicode_buffer(n + 2)
    u.GetWindowTextW(hwnd, buf, n + 2)
    return buf.value


def find_window(title="Anvil"):
    found = [0]

    def cb(hwnd, _):
        if _text(hwnd) == title:
            found[0] = hwnd
        return True

    u.EnumWindows(WNDENUMPROC(cb), 0)
    return found[0]


class BITMAPINFOHEADER(ctypes.Structure):
    _fields_ = [
        ("biSize", wintypes.DWORD),
        ("biWidth", wintypes.LONG),
        ("biHeight", wintypes.LONG),
        ("biPlanes", wintypes.WORD),
        ("biBitCount", wintypes.WORD),
        ("biCompression", wintypes.DWORD),
        ("biSizeImage", wintypes.DWORD),
        ("biXPelsPerMeter", wintypes.LONG),
        ("biYPelsPerMeter", wintypes.LONG),
        ("biClrUsed", wintypes.DWORD),
        ("biClrImportant", wintypes.DWORD),
    ]


def _chunk(tag, data):
    return (
        struct.pack(">I", len(data))
        + tag
        + data
        + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)
    )


def capture(hwnd, path):
    u.ShowWindow(hwnd, 9)
    u.SetForegroundWindow(hwnd)
    time.sleep(0.8)

    rect = wintypes.RECT()
    u.GetWindowRect(hwnd, ctypes.byref(rect))
    width = rect.right - rect.left
    height = rect.bottom - rect.top
    if width <= 0 or height <= 0:
        raise RuntimeError("窗口尺寸为 0，可能已退出")

    screen = u.GetDC(0)
    mem = g.CreateCompatibleDC(screen)
    bmp = g.CreateCompatibleBitmap(screen, width, height)
    old = g.SelectObject(mem, bmp)
    g.BitBlt(mem, 0, 0, width, height, screen, rect.left, rect.top, 0x00CC0020)
    # 必须先把位图从 DC 里摘出来，否则 GetDIBits 只会填一部分，其余全是黑
    g.SelectObject(mem, old)
    g.DeleteDC(mem)

    header = BITMAPINFOHEADER()
    header.biSize = ctypes.sizeof(BITMAPINFOHEADER)
    header.biWidth = width
    header.biHeight = -height
    header.biPlanes = 1
    header.biBitCount = 32
    buffer = ctypes.create_string_buffer(width * height * 4)
    g.GetDIBits(screen, bmp, 0, height, buffer, ctypes.byref(header), 0)
    g.DeleteObject(bmp)
    u.ReleaseDC(0, screen)
    raw = buffer.raw

    rows = []
    for y in range(height):
        line = raw[y * width * 4:(y + 1) * width * 4]
        px = bytearray([0])
        for x in range(width):
            px += bytes((line[x * 4 + 2], line[x * 4 + 1], line[x * 4], 255))
        rows.append(bytes(px))

    png = b"\x89PNG\r\n\x1a\n"
    png += _chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
    png += _chunk(b"IDAT", zlib.compress(b"".join(rows), 6))
    png += _chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(png)
    return width, height


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "shot.png"
    hwnd = find_window()
    if not hwnd:
        print("没找到 Anvil 窗口")
        return 1
    w, h = capture(hwnd, out)
    print(f"saved {out} {w}x{h}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
