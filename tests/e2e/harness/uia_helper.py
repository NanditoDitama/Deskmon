"""Windows Win32 + UIA automation helper for Deskmon E2E tests.

Uses a hybrid ``win32gui.EnumWindows`` + ``pywinauto.Application(backend="uia")``
approach because ``Desktop(backend="uia").windows()`` returns zero windows in
isolated/sandboxed sessions while ``Application(backend="uia").connect(process=pid)
.window(handle=hwnd)`` exposes the full QML UIA tree and supports focus-free
``ValuePattern`` (``set_edit_text``) and ``InvokePattern`` (``invoke``).
"""

from __future__ import annotations

import ctypes
import os
import re
import sys
import tempfile
import time
from pathlib import Path
from typing import Any

import win32con
import win32gui
import win32process

_HELPER_TITLE_EXACT = {
    "MSCTFIME UI",
    "Default IME",
    "QTrayIconMessageWindow",
}

_HELPER_TITLE_SUBSTRINGS = (
    "_q_titlebar",
    "ThemeChangeObserverWindow",
    "ScreenChangeObserverWindow",
    "QTrayIconMessageWindow",
)

_HELPER_CLASS_EXACT = {
    "IME",
    "MSCTFIME UI",
    "UAC_InputIndicatorOverlayWnd",
    "UAC Input Indicator",
}

_HELPER_CLASS_SUBSTRINGS = (
    "ThemeChangeObserverWindow",
    "ScreenChangeObserverWindow",
    "TrayIconMessageWindowClass",
    "InputIndicator",
)

# Explicit EN (DeskmonV2) / ID (DeskmonV2 & 1.0.3.3) label map from UI surface audit.
UI_LABELS: dict[str, dict[str, list[str]]] = {
    "login": {
        "title": ["Deskmon"],
        "subtitle": ["Sign in to track your activity", "Masuk untuk melacak aktivitas Anda"],
        "username": ["Username"],
        "password": ["Password"],
        "sign_in": ["Sign In", "Masuk"],
    },
    "header": {
        "profile": ["Profile", "Profil"],
        "logout": ["Logout", "Keluar"],
        "check_update": ["Check Update", "Cek Update", "Periksa Pembaruan"],
        "settings": ["Settings", "Pengaturan"],
        "refresh": ["Refresh", "Muat Ulang"],
    },
    "task": {
        "pause": ["Pause", "Jeda"],
        "resume": ["Resume", "Lanjutkan"],
        "view_all": ["View All", "Lihat Semua"],
        "need_review": ["Mark as Need Review", "Tandai Butuh Review"],
        "switch_confirm": ["Switch Project", "Beralih ke Proyek"],
        "cancel": ["Cancel", "Batal"],
    },
    "early_leave": {
        "title": ["Early Clock Out", "Keluar Lebih Awal"],
        "submit": ["Submit", "Kirim"],
        "cancel": ["Cancel", "Batal"],
    },
    "task_details": {
        "title": ["Task Details", "Detail Pekerjaan"],
        "submit": ["Submit", "Kirim"],
        "skip": ["Skip", "Lewati"],
    },
    "failure_dialog": {
        "title": ["An Error Occurred", "Terjadi Kesalahan"],
        "retry": ["Retry", "Coba Lagi"],
        "later": ["Later", "Nanti"],
    },
    "need_review": {
        "title": ["Review Request Reason", "Alasan Permintaan Review"],
        "submit": ["Submit", "Kirim"],
        "cancel": ["Cancel", "Batal"],
    },
    "time_up": {
        "title": ["Warning", "Peringatan"],
        "close": ["Close", "Tutup"],
    },
    "auth_error": {
        "title": ["Session Expired", "Sesi Berakhir"],
        "back_to_login": ["Back to Login Page", "Kembali ke halaman Login"],
    },
    "ping_error": {
        "title": ["Connection Error", "Kesalahan Koneksi"],
        "ok": ["OK"],
    },
    "update_dialog": {
        "title": ["Update Available", "Pembaruan Tersedia"],
        "later": ["Later", "Nanti"],
        "update_now": ["Update Now", "Update Sekarang"],
    },
    "idle_notification": {
        "title": ["Idle Detected", "Idle Terdeteksi"],
        "dismiss": ["Dismiss", "Abaikan"],
        "resume": ["Resume", "Lanjutkan"],
    },
    "monitored_apps": {
        "request": ["Request"],
        "add_app": ["Add Application", "Tambah Aplikasi"],
        "ok": ["OK"],
    },
    "add_app_request": {
        "title": ["Add Productivity Application", "Tambah Aplikasi Produktivitas"],
        "add": ["Add", "Tambahkan"],
        "cancel": ["Cancel", "Batal"],
    },
    "pending_requests": {
        "title": ["Application Requests"],
        "close": ["Close", "Tutup"],
    },
    "date_range": {
        "apply": ["Apply", "Terapkan"],
        "cancel": ["Cancel", "Batal"],
        "clear": ["Clear", "Hapus"],
    },
    "crop_image": {
        "save": ["Save", "Simpan"],
        "cancel": ["Cancel", "Batal"],
    },
}

_PYWINAUTO_MODULE: Any = None


def _ensure_pywinauto() -> Any:
    """Redirect comtypes generated wrappers to a writable temp dir before importing pywinauto."""
    global _PYWINAUTO_MODULE
    if _PYWINAUTO_MODULE is not None:
        return _PYWINAUTO_MODULE

    if not hasattr(sys, "coinit_flags"):
        sys.coinit_flags = 2  # COINIT_APARTMENTTHREADED (STA) expected by pywinauto UIA

    import comtypes
    import comtypes.client
    import comtypes.gen

    gen_dir = Path(tempfile.gettempdir()) / "deskmon_e2e_comtypes_gen"
    gen_dir.mkdir(parents=True, exist_ok=True)
    comtypes.client.gen_dir = str(gen_dir)
    comtypes.gen.__path__ = [str(gen_dir)]

    import pywinauto

    _PYWINAUTO_MODULE = pywinauto
    return pywinauto


def uia_available() -> bool:
    """Return True if pywinauto UIA backend initializes without permission/typelib errors."""
    try:
        pywinauto = _ensure_pywinauto()
        _ = pywinauto.Desktop(backend="uia")
        return True
    except Exception:
        return False


def has_interactive_desktop() -> bool:
    """Return True when running with a visible Windows desktop (non-offscreen/non-headless)."""
    qpa = os.environ.get("QT_QPA_PLATFORM", "").strip().lower()
    if qpa == "offscreen":
        return False
    if os.environ.get("DESKMON_E2E_HEADLESS", "").strip() in ("1", "true", "True"):
        return False
    try:
        if win32gui.GetDesktopWindow() == 0:
            return False
        user32 = ctypes.windll.user32  # type: ignore[attr-defined]
        kernel32 = ctypes.windll.kernel32  # type: ignore[attr-defined]
        hdesk = user32.GetThreadDesktop(kernel32.GetCurrentThreadId())
        if not hdesk:
            return False
        # UOI_NAME = 2
        buf = ctypes.create_unicode_buffer(128)
        needed = ctypes.c_uint32(0)
        ok = user32.GetUserObjectInformationW(hdesk, 2, buf, ctypes.sizeof(buf), ctypes.byref(needed))
        return bool(ok and buf.value)
    except Exception:
        return False


def _is_helper_window(title: str, class_name: str) -> bool:
    if title in _HELPER_TITLE_EXACT or class_name in _HELPER_CLASS_EXACT:
        return True
    if any(sub in title for sub in _HELPER_TITLE_SUBSTRINGS):
        return True
    if any(sub in class_name for sub in _HELPER_CLASS_SUBSTRINGS):
        return True
    return False


def find_windows_for_pid(
    pid: int,
    visible_only: bool = False,
    include_helpers: bool = False,
) -> list[dict[str, Any]]:
    """Enumerate top-level Win32 windows belonging to ``pid``."""
    results: list[dict[str, Any]] = []

    def _callback(hwnd: int, _: Any) -> bool:
        try:
            _, win_pid = win32process.GetWindowThreadProcessId(hwnd)
            if win_pid != pid:
                return True
            visible = bool(win32gui.IsWindowVisible(hwnd))
            if visible_only and not visible:
                return True
            title = win32gui.GetWindowText(hwnd) or ""
            class_name = win32gui.GetClassName(hwnd) or ""
            if not include_helpers and _is_helper_window(title, class_name):
                return True
            results.append(
                {
                    "hwnd": int(hwnd),
                    "title": title,
                    "class_name": class_name,
                    "visible": visible,
                }
            )
        except Exception:
            pass
        return True

    win32gui.EnumWindows(_callback, None)
    return results


def wait_for_window(
    pid: int,
    title_re: str | None = None,
    timeout: float = 10.0,
    visible_only: bool = True,
    poll_interval: float = 0.1,
) -> dict[str, Any]:
    """Poll ``find_windows_for_pid`` until a matching window appears."""
    compiled = re.compile(title_re) if title_re is not None else None
    deadline = time.monotonic() + timeout
    last_seen: list[dict[str, Any]] = []

    while time.monotonic() < deadline:
        wins = find_windows_for_pid(pid, visible_only=visible_only)
        last_seen = wins
        for w in wins:
            if compiled is None:
                if w["title"].strip():
                    return w
            elif compiled.search(w["title"]):
                return w
        time.sleep(poll_interval)

    raise TimeoutError(
        f"Timed out after {timeout:.1f}s waiting for window "
        f"(pid={pid}, title_re={title_re!r}, visible_only={visible_only}). "
        f"Last seen: {last_seen!r}"
    )


def _is_valid_hwnd(hwnd: Any) -> bool:
    if isinstance(hwnd, bool):
        return False
    try:
        hwnd_int = int(hwnd)
    except (TypeError, ValueError):
        return False
    return hwnd_int > 0 and hwnd_int != 0xFFFF and bool(win32gui.IsWindow(hwnd_int))


def connect_window(pid_or_hwnd: int, hwnd: int | None = None) -> Any:
    """Connect pywinauto UIA WindowSpecification using hybrid PID + HWND lookup."""
    if hwnd is None:
        if not _is_valid_hwnd(pid_or_hwnd):
            raise ValueError(f"Invalid window handle: {pid_or_hwnd!r}")
        target_hwnd = int(pid_or_hwnd)
        _, pid = win32process.GetWindowThreadProcessId(target_hwnd)
        if pid <= 0:
            raise ValueError(f"Unable to resolve valid PID for window handle: {target_hwnd}")
    else:
        if not _is_valid_hwnd(hwnd):
            raise ValueError(f"Invalid window handle: {hwnd!r}")
        pid = int(pid_or_hwnd)
        target_hwnd = int(hwnd)

    pywinauto = _ensure_pywinauto()
    app = pywinauto.Application(backend="uia").connect(process=pid)
    return app.window(handle=target_hwnd)


connect_uia_window = connect_window


def find_controls(
    window: Any,
    title: str | list[str] | None = None,
    title_re: str | None = None,
    control_type: str | None = None,
) -> list[Any]:
    """Search UIA descendants by control_type and/or accessible text."""
    kwargs: dict[str, Any] = {}
    if control_type is not None:
        kwargs["control_type"] = control_type

    descendants = window.descendants(**kwargs)
    compiled = re.compile(title_re) if title_re is not None else None
    titles_set: set[str] | None = None
    if isinstance(title, str):
        titles_set = {title}
    elif isinstance(title, (list, tuple, set)):
        titles_set = set(title)

    matched: list[Any] = []
    for ctrl in descendants:
        try:
            text = ctrl.window_text() or ""
        except Exception:
            text = ""
        if titles_set is not None and text not in titles_set:
            continue
        if compiled is not None and not compiled.search(text):
            continue
        matched.append(ctrl)
    return matched


def find_buttons(
    window: Any,
    title: str | list[str] | None = None,
    title_re: str | None = None,
) -> list[Any]:
    return find_controls(window, title=title, title_re=title_re, control_type="Button")


def find_edits(window: Any) -> list[Any]:
    return find_controls(window, control_type="Edit")


def find_texts(
    window: Any,
    title: str | list[str] | None = None,
    title_re: str | None = None,
) -> list[Any]:
    return find_controls(window, title=title, title_re=title_re, control_type="Text")


def set_text(edit_ctrl: Any, value: str) -> None:
    """Set text on a QML Edit control using focus-free ValuePattern primary."""
    try:
        edit_ctrl.set_edit_text(value)
    except Exception:
        edit_ctrl.set_text(value)


def click_button(
    window_or_ctrl: Any,
    label: str | list[str] | None = None,
) -> None:
    """Invoke a UIA Button control (either directly or looked up by label)."""
    if label is None:
        ctrl = window_or_ctrl
    else:
        matches = find_buttons(window_or_ctrl, title=label)
        if not matches:
            available = []
            for b in find_buttons(window_or_ctrl):
                try:
                    available.append(b.window_text())
                except Exception:
                    pass
            raise LookupError(
                f"Button with label {label!r} not found. Available buttons: {available!r}"
            )
        ctrl = matches[0]

    try:
        ctrl.invoke()
    except Exception:
        ctrl.click_input()


def close_window(hwnd: int) -> bool:
    """Request graceful window close via WM_CLOSE (triggers QML Window.onClosing)."""
    if not _is_valid_hwnd(hwnd):
        return False
    try:
        win32gui.PostMessage(int(hwnd), win32con.WM_CLOSE, 0, 0)
        return True
    except Exception:
        return False
