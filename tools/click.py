"""点击 Anvil 窗口内指定颜色的控件（默认找主按钮的蓝色），用于自动化回归。"""

import ctypes
import sys
import time
from ctypes import wintypes

u = ctypes.WinDLL("user32", use_last_error=True)
g = ctypes.WinDLL("gdi32", use_last_error=True)
u.SetProcessDpiAwarenessContext(ctypes.c_void_p(-4))

WNDENUMPROC = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)


class RECT(ctypes.Structure):
    _fields_ = [("left", ctypes.c_long), ("top", ctypes.c_long), ("right", ctypes.c_long), ("bottom", ctypes.c_long)]


class BITMAPINFOHEADER(ctypes.Structure):
    _fields_ = [
        ("biSize", wintypes.DWORD), ("biWidth", wintypes.LONG), ("biHeight", wintypes.LONG),
        ("biPlanes", wintypes.WORD), ("biBitCount", wintypes.WORD), ("biCompression", wintypes.DWORD),
        ("biSizeImage", wintypes.DWORD), ("biXPelsPerMeter", wintypes.LONG), ("biYPelsPerMeter", wintypes.LONG),
        ("biClrUsed", wintypes.DWORD), ("biClrImportant", wintypes.DWORD),
    ]


def find_window(title="Anvil"):
    found = [0]

    def cb(hwnd, _):
        n = u.GetWindowTextLengthW(hwnd)
        buf = ctypes.create_unicode_buffer(n + 2)
        u.GetWindowTextW(hwnd, buf, n + 2)
        if buf.value == title:
            found[0] = hwnd
        return True

    u.EnumWindows(WNDENUMPROC(cb), 0)
    return found[0]


def grab(hwnd):
    rect = RECT()
    u.GetWindowRect(hwnd, ctypes.byref(rect))
    width = rect.right - rect.left
    height = rect.bottom - rect.top
    screen = u.GetDC(0)
    mem = g.CreateCompatibleDC(screen)
    bmp = g.CreateCompatibleBitmap(screen, width, height)
    old = g.SelectObject(mem, bmp)
    g.BitBlt(mem, 0, 0, width, height, screen, rect.left, rect.top, 0x00CC0020)
    g.SelectObject(mem, old)
    g.DeleteDC(mem)

    header = BITMAPINFOHEADER()
    header.biSize = ctypes.sizeof(BITMAPINFOHEADER)
    header.biWidth = width
    header.biHeight = -height
    header.biPlanes = 1
    header.biBitCount = 32
    buf = ctypes.create_string_buffer(width * height * 4)
    g.GetDIBits(screen, bmp, 0, height, buf, ctypes.byref(header), 0)
    g.DeleteObject(bmp)
    u.ReleaseDC(0, screen)
    return rect, width, height, buf.raw


def main():
    target = (0x25, 0x63, 0xEB)
    if len(sys.argv) >= 4:
        target = tuple(int(v, 16) for v in sys.argv[1:4])

    hwnd = find_window()
    if not hwnd:
        print("没找到 Anvil 窗口")
        return 1
    u.ShowWindow(hwnd, 9)
    u.SetForegroundWindow(hwnd)
    time.sleep(0.5)

    rect, width, height, raw = grab(hwnd)
    xs, ys = [], []
    for y in range(height):
        row = raw[y * width * 4:(y + 1) * width * 4]
        for x in range(width):
            b, gg, r = row[x * 4], row[x * 4 + 1], row[x * 4 + 2]
            if abs(r - target[0]) < 14 and abs(gg - target[1]) < 14 and abs(b - target[2]) < 14:
                xs.append(x)
                ys.append(y)
    if not xs:
        print("没找到目标颜色")
        return 1

    cx = (min(xs) + max(xs)) // 2
    cy = (min(ys) + max(ys)) // 2
    sx, sy = rect.left + cx, rect.top + cy
    print(f"控件区域 {min(xs)},{min(ys)} - {max(xs)},{max(ys)} -> 点击 {sx},{sy}")
    u.SetCursorPos(sx, sy)
    time.sleep(0.2)
    u.mouse_event(0x0002, 0, 0, 0, 0)
    time.sleep(0.06)
    u.mouse_event(0x0004, 0, 0, 0, 0)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
