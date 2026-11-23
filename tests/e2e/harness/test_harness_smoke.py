"""End-to-end smoke self-tests verifying EM1 E2E test infrastructure."""

from __future__ import annotations

import json
import os
import subprocess
import sys
import threading
import time
import unittest
import urllib.error
import urllib.request
from pathlib import Path

from PIL import Image

E2E_ROOT = Path(__file__).resolve().parent.parent
REPO_ROOT = E2E_ROOT.parent.parent
if str(E2E_ROOT) not in sys.path:
    sys.path.insert(0, str(E2E_ROOT))

from harness.db_fixtures import (  # noqa: E402
    V2_EXPECTED_TABLES,
    V2_EXPECTED_VIEWS,
    assert_db_parity,
    diff_snapshots,
    read_1033_snapshot,
    read_v2_legacy_views_snapshot,
    read_v2_snapshot,
    schema_check_v2,
    seed_1033_dbs,
    seed_v2_db,
)
from harness.mock_server import MockDeskmonServer  # noqa: E402
from harness.process_env import (  # noqa: E402
    IsolatedDeskmonEnv,
    find_ref_binary,
    preflight_check_host_instance,
)
from harness.uia_helper import (  # noqa: E402
    UI_LABELS,
    close_window,
    connect_window,
    find_buttons,
    find_edits,
    find_texts,
    find_windows_for_pid,
    has_interactive_desktop,
    uia_available,
    wait_for_window,
)


def _http_call(
    url: str,
    method: str = "GET",
    payload: dict | None = None,
    token: str | None = None,
    timeout: float = 5.0,
) -> tuple[int, bytes, dict[str, str]]:
    headers: dict[str, str] = {}
    data: bytes | None = None
    if payload is not None:
        data = json.dumps(payload).encode("utf-8")
        headers["Content-Type"] = "application/json"
    if token is not None:
        headers["Authorization"] = f"Bearer {token}"
    req = urllib.request.Request(url, data=data, headers=headers, method=method)
    try:
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            return resp.status, resp.read(), {k.lower(): v for k, v in resp.headers.items()}
    except urllib.error.HTTPError as err:
        return err.code, err.read(), {k.lower(): v for k, v in err.headers.items()}


def _resolve_v2_bin() -> Path:
    env_bin = os.environ.get("DESKMON_E2E_V2_BIN", "").strip()
    if env_bin:
        return Path(env_bin)
    return REPO_ROOT / "build_e2e" / "Deskmon.exe"


class TestHarnessSmoke(unittest.TestCase):
    def test_mock_server_routes_auth_and_schemas(self) -> None:
        with MockDeskmonServer() as mock:
            self.assertTrue(mock.base_api_url.startswith("http://127.0.0.1:"))
            self.assertTrue(mock.update_url.endswith("/version.json"))

            # Unauthenticated request must return 401 Host requires authentication
            status, body, _ = _http_call(f"{mock.base_api_url}/task-by-user/1")
            self.assertEqual(status, 401)
            self.assertEqual(json.loads(body)["message"], "Host requires authentication")

            # Login issues token
            status, body, _ = _http_call(
                f"{mock.base_api_url}/login",
                method="POST",
                payload={"email": "testuser@pranala-dt.co.id", "password": "secret123"},
            )
            self.assertEqual(status, 200)
            login_json = json.loads(body)
            self.assertTrue(login_json["success"])
            token = login_json["token"]

            # Authenticated GET endpoints
            for endpoint in (
                "/task-by-user/1",
                "/get-current-task-status/10",
                "/app-request/all",
                "/get-time-at-work/1",
                "/get-check-in/1",
            ):
                s, b, _ = _http_call(f"{mock.base_api_url}{endpoint}", token=token)
                self.assertEqual(s, 200, f"{endpoint} failed: {b!r}")
                self.assertTrue(json.loads(b)["success"])

            # Update manifest and favicon PNG
            s, b, _ = _http_call(mock.update_url)
            self.assertEqual(s, 200)
            self.assertIn("version", json.loads(b))

            s, b, hdrs = _http_call(f"{mock.favicon_url_prefix}?sz=32&domain=github.com")
            self.assertEqual(s, 200)
            self.assertEqual(hdrs.get("content-type"), "image/png")
            self.assertTrue(b.startswith(b"\x89PNG\r\n\x1a\n"))

            # Strict schema validation: /api/ping rejects integer task_id, accepts string task_id
            s, b, _ = _http_call(
                f"{mock.base_api_url}/ping",
                method="POST",
                payload={"task_id": 10},
                token=token,
            )
            self.assertEqual(s, 400)
            self.assertIn("validation_error", json.loads(b))

            s, _, _ = _http_call(
                f"{mock.base_api_url}/ping",
                method="POST",
                payload={"task_id": "10"},
                token=token,
            )
            self.assertEqual(s, 200)

            s, _, _ = _http_call(
                f"{mock.base_api_url}/end-implementation/10",
                method="PUT",
                payload={"status": "stop"},
                token=token,
            )
            self.assertEqual(s, 200)

            # /__mock/requests and /__mock/reset
            s, b, _ = _http_call(f"{mock.base_url}/__mock/requests?path=/api/ping")
            self.assertEqual(s, 200)
            reqs = json.loads(b)["requests"]
            self.assertEqual(len(reqs), 2)
            self.assertIsNotNone(reqs[0]["validation_error"])
            self.assertIsNone(reqs[1]["validation_error"])

            s, _, _ = _http_call(f"{mock.base_url}/__mock/reset", method="POST", payload={})
            self.assertEqual(s, 200)
            self.assertEqual(mock.get_requests(), [])

    def test_mock_server_fault_injection_and_clean_shutdown(self) -> None:
        mock = MockDeskmonServer()
        with mock:
            token = "mock-token-user-1"

            # 1. Single-shot 500 then recovery
            mock.inject_fault({"route": r"^/api/ping$", "method": "POST", "count": 1, "status_code": 500})
            s1, _, _ = _http_call(
                f"{mock.base_api_url}/ping", method="POST", payload={"task_id": "10"}, token=token
            )
            s2, _, _ = _http_call(
                f"{mock.base_api_url}/ping", method="POST", payload={"task_id": "10"}, token=token
            )
            self.assertEqual(s1, 500)
            self.assertEqual(s2, 200)

            # 2. Malformed JSON
            mock.inject_fault({"route": r"^/api/app-request/all$", "count": 1, "mode": "malformed_json"})
            s, raw, _ = _http_call(f"{mock.base_api_url}/app-request/all", token=token)
            self.assertEqual(s, 200)
            with self.assertRaises(json.JSONDecodeError):
                json.loads(raw)

            # 3. JSON override (IP block simulation)
            mock.inject_fault(
                {
                    "route": r"^/api/login$",
                    "count": 1,
                    "mode": "json_override",
                    "json_override": {
                        "success": False,
                        "message": "Login not allowed from this IP address",
                        "your_ip": "203.0.113.42",
                    },
                }
            )
            s, body, _ = _http_call(
                f"{mock.base_api_url}/login",
                method="POST",
                payload={"email": "testuser@pranala-dt.co.id", "password": "secret123"},
            )
            self.assertEqual(s, 200)
            self.assertEqual(json.loads(body)["your_ip"], "203.0.113.42")

            # 4. Drop connection
            mock.inject_fault({"route": r"^/api/get-check-in/\d+$", "count": 1, "mode": "drop_connection"})
            with self.assertRaises(Exception):
                _http_call(f"{mock.base_api_url}/get-check-in/1", token=token)

            # 5. Interruptible long delay during shutdown
            mock.inject_fault({"route": r"^/api/ping$", "count": 1, "mode": "delay_ms", "delay_ms": 15000})
            bg_errors: list[BaseException] = []

            def _slow_req() -> None:
                try:
                    _http_call(
                        f"{mock.base_api_url}/ping",
                        method="POST",
                        payload={"task_id": "10"},
                        token=token,
                        timeout=20.0,
                    )
                except BaseException as exc:
                    bg_errors.append(exc)

            t = threading.Thread(target=_slow_req, daemon=True)
            t.start()
            time.sleep(0.15)
            t_stop_start = time.monotonic()

        elapsed = time.monotonic() - t_stop_start
        t.join(timeout=2.0)
        self.assertLess(elapsed, 2.0, f"Server stop took too long: {elapsed:.2f}s")
        self.assertTrue(len(bg_errors) >= 1)

    def test_db_fixtures_schemas_and_snapshot_diff(self) -> None:
        with IsolatedDeskmonEnv() as env:
            act_db = env.cwd / "activity_logs.db"
            prod_db = env.cwd / "produktif_app_db.db"
            v2_db = env.db_path

            dataset = {
                "users": [
                    {
                        "id": 1,
                        "username": "testuser",
                        "password": "secret123",
                        "email": "testuser@pranala-dt.co.id",
                        "department": "Engineering",
                        "role": "Developer",
                        "token": "mock-token-user-1",
                        "profile_image": None,
                    }
                ],
                "logs": [
                    {
                        "id": 1,
                        "user_id": 1,
                        "start_time": 1700000000,
                        "end_time": 1700000120,
                        "app_name": "Code",
                        "title": "main.cpp",
                        "url": "",
                    }
                ],
                "apps": [
                    {
                        "id": 1,
                        "app_name": "Visual Studio Code",
                        "window_title": "",
                        "url": "",
                        "productivity_type": 1,
                        "productivity": 1,
                        "for_user": "0",
                    }
                ],
                "tasks": [
                    {
                        "id": 10,
                        "user_id": 1,
                        "project_name": "Project Alpha",
                        "task_desc": "Implement feature A",
                        "max_time": 14400,
                        "time_usage": 1800,
                        "active": 1,
                        "status": "pending",
                        "paused": 0,
                        "created_at": "2026-11-21T08:00:00Z",
                    }
                ],
                "completed_tasks": [],
                "idle_settings": [{"id": 1, "threshold_seconds": 180}],
                "pause_logs": [
                    {
                        "id": 1,
                        "task_id": 10,
                        "paused_at": "2026-11-21T09:00:00Z",
                        "resumed_at": "2026-11-21T09:05:00Z",
                        "status": "resumed",
                    }
                ],
                "work_time": [
                    {
                        "id": 1,
                        "user_id": 1,
                        "work_date": "2026-11-21",
                        "elapsed_seconds": 3600,
                    }
                ],
            }

            seed_1033_dbs(act_db, prod_db, dataset)
            seed_v2_db(v2_db, dataset)

            schema = schema_check_v2(v2_db)
            self.assertTrue(V2_EXPECTED_TABLES.issubset(schema["tables"]))
            self.assertTrue(V2_EXPECTED_VIEWS.issubset(schema["views"]))
            self.assertIn("productivity", schema["columns"]["productivity_apps"])

            snap_1033 = read_1033_snapshot(act_db, prod_db)
            snap_v2 = read_v2_snapshot(v2_db)
            self.assertEqual(diff_snapshots(snap_1033, snap_v2), [])
            assert_db_parity(snap_1033, snap_v2)

            views_snap = read_v2_legacy_views_snapshot(v2_db)
            self.assertEqual(len(views_snap["log"]), 1)
            self.assertEqual(views_snap["log"][0]["id_user"], 1)
            self.assertEqual(views_snap["aplikasi"][0]["aplikasi"], "Visual Studio Code")
            self.assertEqual(views_snap["task"][0]["task"], "Implement feature A")
            self.assertEqual(views_snap["work_time"][0]["date"], "2026-11-21")

    def test_uia_helper_capabilities_and_sample_avatar(self) -> None:
        self.assertIsInstance(uia_available(), bool)
        self.assertIsInstance(has_interactive_desktop(), bool)
        wins = find_windows_for_pid(os.getpid())
        self.assertIsInstance(wins, list)

        avatar_path = E2E_ROOT / "fixtures" / "sample_avatar.png"
        self.assertTrue(avatar_path.is_file(), f"Missing fixture: {avatar_path}")
        raw = avatar_path.read_bytes()
        self.assertTrue(raw.startswith(b"\x89PNG\r\n\x1a\n"))
        with Image.open(avatar_path) as img:
            self.assertGreaterEqual(img.width, 256)
            self.assertGreaterEqual(img.height, 256)
            self.assertEqual(img.mode, "RGBA")

    def test_process_env_launch_deskmon_v2_isolated(self) -> None:
        v2_bin = _resolve_v2_bin()
        self.assertTrue(v2_bin.is_file(), f"Built DeskmonV2 binary not found at {v2_bin}")

        temp_root: Path
        launched_proc = None
        with MockDeskmonServer() as mock:
            with IsolatedDeskmonEnv(
                api_base_url=mock.base_api_url,
                update_url=mock.update_url,
                qt_qpa_platform="offscreen",
            ) as env:
                temp_root = env.root
                proc = env.launch(v2_bin)
                launched_proc = proc
                proc.wait_for_log(r"Deskmon Database Initialized", timeout=15.0)
                self.assertTrue(env.db_path.is_file(), f"Isolated DB not created at {env.db_path}")
                schema = schema_check_v2(env.db_path)
                self.assertTrue(V2_EXPECTED_TABLES.issubset(schema["tables"]))
                self.assertTrue(V2_EXPECTED_VIEWS.issubset(schema["views"]))

        self.assertIsNotNone(launched_proc)
        self.assertFalse(launched_proc.is_running())
        self.assertFalse(temp_root.exists())

    def test_process_env_preflight_and_ref_locator(self) -> None:
        is_clean, detail = preflight_check_host_instance("DeskmonAppInstance")
        self.assertIsInstance(is_clean, bool)
        self.assertIsInstance(detail, str)
        self.assertTrue(len(detail) > 0)

        ref_bin = find_ref_binary()
        self.assertTrue(ref_bin is None or isinstance(ref_bin, Path))

    def test_process_env_launch_deskmon_v2_windows_uia_login(self) -> None:
        if not has_interactive_desktop() or not uia_available():
            self.skipTest("Interactive Windows desktop / UIA backend unavailable in current environment")

        v2_bin = _resolve_v2_bin()
        self.assertTrue(v2_bin.is_file(), f"Built DeskmonV2 binary not found at {v2_bin}")

        with MockDeskmonServer() as mock:
            with IsolatedDeskmonEnv(
                api_base_url=mock.base_api_url,
                update_url=mock.update_url,
                qt_qpa_platform="windows",
            ) as env:
                proc = env.launch(v2_bin)
                proc.wait_for_log(r"Deskmon Database Initialized", timeout=15.0)

                win_info = wait_for_window(proc.pid, title_re=r"Deskmon", timeout=15.0, visible_only=True)
                hwnd = win_info["hwnd"]
                uia_win = connect_window(proc.pid, hwnd)

                texts = []
                for t in find_texts(uia_win):
                    try:
                        texts.append(t.window_text())
                    except Exception:
                        pass

                edits = find_edits(uia_win)
                buttons = find_buttons(uia_win, title=UI_LABELS["login"]["sign_in"])

                self.assertIn("Deskmon", texts)
                self.assertTrue(
                    any(lbl in texts for lbl in UI_LABELS["login"]["subtitle"]),
                    f"Login subtitle not found in UIA texts: {texts!r}",
                )
                self.assertGreaterEqual(len(edits), 2, f"Expected >=2 Edit controls, got {len(edits)}")
                self.assertGreaterEqual(len(buttons), 1, "Expected Sign In button in UIA tree")

                # Verify WM_CLOSE triggers close-to-tray (hides window while process stays alive)
                close_window(hwnd)
                deadline = time.monotonic() + 5.0
                visible_now = find_windows_for_pid(proc.pid, visible_only=True)
                while visible_now and time.monotonic() < deadline:
                    time.sleep(0.1)
                    visible_now = find_windows_for_pid(proc.pid, visible_only=True)
                self.assertEqual(visible_now, [], f"Window remained visible after WM_CLOSE: {visible_now!r}")
                self.assertTrue(proc.is_running(), "Process exited on close instead of hiding to tray")

    def test_mock_server_strict_validation_and_fault_count_edge_cases(self) -> None:
        with MockDeskmonServer() as mock:
            token = "mock-token-user-1"

            # Reject empty data: [] on POST /api/productivity-app under strict_schemas=True
            s, b, _ = _http_call(
                f"{mock.base_api_url}/productivity-app",
                method="POST",
                payload={"data": []},
                token=token,
            )
            self.assertEqual(s, 400)
            self.assertIn("validation_error", json.loads(b))

            # Reject whitespace-padded alasan exceeding 500 raw chars on POST /api/update-status-task
            padded_alasan = (" " * 10) + "a" + (" " * 500)
            self.assertEqual(len(padded_alasan), 511)
            s, b, _ = _http_call(
                f"{mock.base_api_url}/update-status-task/10/1",
                method="POST",
                payload={"status": "need-review", "alasan": padded_alasan},
                token=token,
            )
            self.assertEqual(s, 400)
            self.assertIn("validation_error", json.loads(b))

            # Fault with count=0 must fire 0 times; count<0 raises ValueError
            mock.inject_fault(
                {"route": r"^/api/ping$", "method": "POST", "count": 0, "status_code": 502}
            )
            s, _, _ = _http_call(
                f"{mock.base_api_url}/ping",
                method="POST",
                payload={"task_id": "10"},
                token=token,
            )
            self.assertEqual(s, 200)

            with self.assertRaises(ValueError):
                mock.inject_fault({"route": r"^/api/ping$", "count": -1, "status_code": 502})

    def test_uia_helper_invalid_hwnd_guards(self) -> None:
        for bad_hwnd in (99999999, -1, 0, 0xFFFF):
            self.assertFalse(close_window(bad_hwnd))
            with self.assertRaises(ValueError):
                connect_window(bad_hwnd)
            with self.assertRaises(ValueError):
                connect_window(os.getpid(), bad_hwnd)

    def test_process_env_kills_grandchild_process_tree_on_exit(self) -> None:
        def _pid_alive(pid: int) -> bool:
            res = subprocess.run(
                ["tasklist", "/FI", f"PID eq {pid}", "/FO", "CSV", "/NH"],
                capture_output=True,
                text=True,
                timeout=5,
                check=False,
            )
            return f'"{pid}"' in res.stdout

        child_code = (
            "import subprocess, sys, time\n"
            "gc = subprocess.Popen([sys.executable, '-c', 'import time; time.sleep(60)'])\n"
            "print(f'GRANDCHILD_PID={gc.pid}', flush=True)\n"
            "time.sleep(60)\n"
        )

        grandchild_pid: int | None = None
        child_proc = None
        try:
            with self.assertRaises(RuntimeError):
                with IsolatedDeskmonEnv() as env:
                    child_proc = env.launch(sys.executable, args=["-c", child_code])
                    m = child_proc.wait_for_log(r"GRANDCHILD_PID=(\d+)", timeout=10.0)
                    grandchild_pid = int(m.group(1))
                    self.assertTrue(_pid_alive(grandchild_pid), "Grandchild did not start")
                    raise RuntimeError("simulated test failure")

            self.assertIsNotNone(child_proc)
            self.assertFalse(child_proc.is_running())
            self.assertIsNotNone(grandchild_pid)

            deadline = time.monotonic() + 3.0
            while _pid_alive(grandchild_pid) and time.monotonic() < deadline:
                time.sleep(0.05)
            self.assertFalse(
                _pid_alive(grandchild_pid),
                f"Grandchild PID {grandchild_pid} leaked after IsolatedDeskmonEnv.__exit__()",
            )
        finally:
            if grandchild_pid is not None and _pid_alive(grandchild_pid):
                subprocess.run(
                    ["taskkill", "/PID", str(grandchild_pid), "/T", "/F"],
                    capture_output=True,
                    check=False,
                )
