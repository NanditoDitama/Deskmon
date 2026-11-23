"""Local mock HTTP server for DeskmonV2 and Deskmon 1.0.3.3 E2E testing.

Implements all 16 ``/api/*`` endpoints, ``/version.json``, ``/s2/favicons``,
and ``/__mock/*`` control endpoints using Python stdlib ``http.server`` with
strict schema validation and interruptible fault injection.
"""

from __future__ import annotations

import copy
import datetime as _dt
import http.server
import json
import re
import socket
import struct
import threading
import time
import urllib.parse
import uuid
import zlib
from typing import Any


def _make_1x1_png() -> bytes:
    """Build a valid 1x1 RGBA PNG byte string using stdlib zlib/struct."""
    def _chunk(tag: bytes, payload: bytes) -> bytes:
        crc = zlib.crc32(tag + payload) & 0xFFFFFFFF
        return struct.pack("!I", len(payload)) + tag + payload + struct.pack("!I", crc)

    ihdr = struct.pack("!IIBBBBB", 1, 1, 8, 6, 0, 0, 0)
    raw_scanline = b"\x00\x3b\x82\xf6\xff"
    idat = zlib.compress(raw_scanline)
    return (
        b"\x89PNG\r\n\x1a\n"
        + _chunk(b"IHDR", ihdr)
        + _chunk(b"IDAT", idat)
        + _chunk(b"IEND", b"")
    )


_FAVICON_PNG_BYTES = _make_1x1_png()


def _is_int(value: Any) -> bool:
    return isinstance(value, int) and not isinstance(value, bool)


def _is_number(value: Any) -> bool:
    return isinstance(value, (int, float)) and not isinstance(value, bool)


def _is_non_empty_str(value: Any) -> bool:
    return isinstance(value, str) and len(value.strip()) > 0


def _today_iso_utc() -> str:
    return _dt.datetime.now(_dt.timezone.utc).strftime("%Y-%m-%dT08:00:00Z")


def _default_state() -> dict[str, Any]:
    today_iso = _today_iso_utc()
    return {
        "enforce_auth": True,
        "blocked_ips": [],
        "users": [
            {
                "id": 1,
                "name": "testuser",
                "email": "testuser@pranala-dt.co.id",
                "password": "secret123",
                "role": {"rolename": "Developer"},
                "department": {"rolename": "Engineering"},
                "token": "mock-token-user-1",
                "blocked_ip": None,
            }
        ],
        "tasks": [
            {
                "id": 10,
                "title": "Project Alpha",
                "description": "Implement feature A",
                "user_id": 1,
                "status": "pending",
                "duration": 4.0,
                "total_duration": 0.5,
                "created_at": today_iso,
            },
            {
                "id": 11,
                "title": "Project Beta",
                "description": "Fix bug B",
                "user_id": 1,
                "status": "need-revise",
                "duration": 8.0,
                "total_duration": 0.0,
                "created_at": today_iso,
            },
        ],
        "task_statuses": {
            "10": "pending",
            "11": "need-revise",
        },
        "apps": [
            {
                "application_name": "Visual Studio Code",
                "process_name": "Code",
                "url": "",
                "productivity_status": "productive",
                "user_id": 0,
            },
            {
                "application_name": "YouTube",
                "process_name": "chrome",
                "url": "youtube.com",
                "productivity_status": "non-productive",
                "user_id": 0,
            },
        ],
        "work_time_raw": 0,
        "work_time_by_user": {"1": 0},
        "check_in": {"clock_in": "08:59", "clock_out": None},
        "version_manifest": {
            "version": "1.0.3.4",
            "releaseNotes": "Current release",
            "downloadUrl": "https://example.invalid/DeskmonSetup.exe",
        },
        "early_leave_reasons": [],
        "task_details": [],
        "submitted_app_requests": [],
        "usage_reports": [],
    }


class _DeskmonThreadingHTTPServer(http.server.ThreadingHTTPServer):
    daemon_threads = True
    allow_reuse_address = True

    def __init__(
        self,
        server_address: tuple[str, int],
        handler_cls: type[http.server.BaseHTTPRequestHandler],
        owner: "MockDeskmonServer",
    ) -> None:
        self.mock_owner = owner
        self._stop_event = owner._stop_event
        super().__init__(server_address, handler_cls)


class _DeskmonMockHandler(http.server.BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def log_message(self, format: str, *args: Any) -> None:
        # Silence noisy default BaseHTTPRequestHandler stderr logging.
        return

    def do_GET(self) -> None:
        self._dispatch("GET")

    def do_POST(self) -> None:
        self._dispatch("POST")

    def do_PUT(self) -> None:
        self._dispatch("PUT")

    def do_DELETE(self) -> None:
        self._dispatch("DELETE")

    def _read_body(self) -> bytes:
        length_header = self.headers.get("Content-Length")
        if not length_header:
            return b""
        try:
            length = int(length_header)
        except ValueError:
            return b""
        if length <= 0:
            return b""
        return self.rfile.read(length)

    def _send_bytes(
        self,
        status: int,
        content_type: str,
        body: bytes,
        extra_headers: dict[str, str] | None = None,
    ) -> None:
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Connection", "close")
        if extra_headers:
            for k, v in extra_headers.items():
                self.send_header(k, v)
        self.end_headers()
        if body:
            self.wfile.write(body)
        self.wfile.flush()

    def _send_json(
        self,
        status: int,
        payload: Any,
        extra_headers: dict[str, str] | None = None,
    ) -> None:
        raw = json.dumps(payload).encode("utf-8")
        self._send_bytes(status, "application/json", raw, extra_headers=extra_headers)

    def _drop_connection(self) -> None:
        self.close_connection = True
        try:
            self.connection.shutdown(socket.SHUT_RDWR)
        except OSError:
            pass
        try:
            self.connection.close()
        except OSError:
            pass

    def _dispatch(self, method: str) -> None:
        owner: MockDeskmonServer = self.server.mock_owner  # type: ignore[attr-defined]
        parsed = urllib.parse.urlparse(self.path)
        path = parsed.path
        query = parsed.query

        raw_bytes = self._read_body()
        raw_body_str = raw_bytes.decode("utf-8", errors="replace")
        body_json: Any = None
        json_parse_error: str | None = None
        if raw_body_str.strip():
            try:
                body_json = json.loads(raw_body_str)
            except json.JSONDecodeError as exc:
                json_parse_error = str(exc)

        headers_lower = {k.lower(): v for k, v in self.headers.items()}

        # Handle /__mock/* control plane directly without polluting request logs.
        if path.startswith("/__mock/"):
            status, payload = owner._handle_mock_control(
                method, path, query, body_json, json_parse_error
            )
            self._send_json(status, payload)
            return

        # Record request & match any fault rule atomically under lock.
        req_entry, fault = owner._begin_request(
            method=method,
            path=path,
            query=query,
            headers=headers_lower,
            raw_body=raw_body_str,
            body_json=body_json,
        )

        # Execute fault delay OUTSIDE lock so concurrent requests and stop() aren't blocked.
        if fault is not None:
            delay_ms = fault.get("delay_ms") or 0
            if delay_ms > 0:
                stopped = owner._stop_event.wait(delay_ms / 1000.0)
                if stopped:
                    owner._finish_request(req_entry, response_status=None)
                    self._drop_connection()
                    return

            mode = fault.get("mode", "status_code")
            extra_headers = fault.get("headers") or {}
            if mode == "drop_connection":
                owner._finish_request(req_entry, response_status=None)
                self._drop_connection()
                return
            if mode == "malformed_json":
                status = int(fault.get("status_code") or 200)
                raw_out = fault.get("raw_body")
                if raw_out is None:
                    raw_out = "{not-valid-json"
                owner._finish_request(req_entry, response_status=status)
                self._send_bytes(
                    status,
                    "application/json",
                    str(raw_out).encode("utf-8"),
                    extra_headers=extra_headers,
                )
                return
            if mode == "json_override":
                status = int(fault.get("status_code") or 200)
                override_payload = fault.get("json_override")
                owner._finish_request(req_entry, response_status=status)
                self._send_json(status, override_payload, extra_headers=extra_headers)
                return
            if mode == "status_code":
                status = int(fault.get("status_code") or 500)
                override_payload = fault.get("json_override")
                if override_payload is None:
                    override_payload = {
                        "success": False,
                        "message": f"Injected HTTP {status}",
                    }
                owner._finish_request(req_entry, response_status=status)
                self._send_json(status, override_payload, extra_headers=extra_headers)
                return
            # mode == "delay_ms": fall through to normal handler after waiting.

        status, content_type, body_bytes, validation_error = owner._handle_app_route(
            method=method,
            path=path,
            query=query,
            headers=headers_lower,
            raw_body=raw_body_str,
            body_json=body_json,
            json_parse_error=json_parse_error,
        )
        owner._finish_request(
            req_entry,
            response_status=status,
            validation_error=validation_error,
        )
        self._send_bytes(status, content_type, body_bytes)


class MockDeskmonServer:
    """Thread-safe local HTTP mock server implementing all Deskmon API contracts."""

    def __init__(
        self,
        host: str = "127.0.0.1",
        port: int = 0,
        strict_schemas: bool = True,
    ) -> None:
        self.host = host
        self._requested_port = port
        self.strict_schemas = strict_schemas
        self._lock = threading.RLock()
        self._stop_event = threading.Event()
        self._httpd: _DeskmonThreadingHTTPServer | None = None
        self._thread: threading.Thread | None = None
        self._seq = 0
        self._state: dict[str, Any] = {}
        self._active_tokens: dict[str, int] = {}
        self._faults: list[dict[str, Any]] = []
        self._requests: list[dict[str, Any]] = []
        self.reset()

    @property
    def port(self) -> int:
        if self._httpd is None:
            raise RuntimeError("MockDeskmonServer is not started")
        return int(self._httpd.server_address[1])

    @property
    def base_url(self) -> str:
        return f"http://{self.host}:{self.port}"

    @property
    def base_api_url(self) -> str:
        return f"{self.base_url}/api"

    @property
    def update_url(self) -> str:
        return f"{self.base_url}/version.json"

    @property
    def favicon_url_prefix(self) -> str:
        return f"{self.base_url}/s2/favicons"

    def start(self) -> "MockDeskmonServer":
        if self._httpd is not None:
            return self
        self._stop_event.clear()
        self._httpd = _DeskmonThreadingHTTPServer(
            (self.host, self._requested_port), _DeskmonMockHandler, self
        )
        self._thread = threading.Thread(
            target=self._httpd.serve_forever,
            name="MockDeskmonServerThread",
            daemon=True,
        )
        self._thread.start()
        return self

    def stop(self) -> None:
        self._stop_event.set()
        if self._httpd is not None:
            try:
                self._httpd.shutdown()
            except Exception:
                pass
            try:
                self._httpd.server_close()
            except Exception:
                pass
            self._httpd = None
        if self._thread is not None:
            self._thread.join(timeout=3.0)
            self._thread = None

    def __enter__(self) -> "MockDeskmonServer":
        return self.start()

    def __exit__(self, exc_type: Any, exc: Any, tb: Any) -> None:
        self.stop()

    def reset(self) -> None:
        with self._lock:
            self._state = _default_state()
            self._active_tokens = {
                u["token"]: int(u["id"])
                for u in self._state["users"]
                if u.get("token")
            }
            self._faults = []
            self._requests = []

    def set_state(self, patch: dict[str, Any]) -> dict[str, Any]:
        with self._lock:
            for key, value in patch.items():
                self._state[key] = copy.deepcopy(value)
            if "users" in patch:
                self._active_tokens = {
                    u["token"]: int(u["id"])
                    for u in self._state.get("users", [])
                    if u.get("token")
                }
            return copy.deepcopy(self._state)

    def get_state(self) -> dict[str, Any]:
        with self._lock:
            return copy.deepcopy(self._state)

    def inject_fault(self, rule: dict[str, Any]) -> str:
        with self._lock:
            rule_copy = copy.deepcopy(rule)
            rule_id = rule_copy.get("rule_id") or f"fault-{uuid.uuid4().hex[:8]}"
            rule_copy["rule_id"] = rule_id
            count = rule_copy.get("count")
            if count is not None:
                if not _is_int(count):
                    raise ValueError("fault rule 'count' must be an integer or None")
                if count < 0:
                    raise ValueError("fault rule 'count' must be >= 0")
                if count == 0:
                    return rule_id
            if "route" not in rule_copy:
                rule_copy["route"] = r".*"
            if "mode" not in rule_copy:
                if "json_override" in rule_copy and rule_copy["json_override"] is not None and "status_code" not in rule_copy:
                    rule_copy["mode"] = "json_override"
                elif "status_code" in rule_copy and rule_copy["status_code"] is not None:
                    rule_copy["mode"] = "status_code"
                elif "delay_ms" in rule_copy and rule_copy["delay_ms"]:
                    rule_copy["mode"] = "delay_ms"
                else:
                    rule_copy["mode"] = "status_code"
            self._faults.append(rule_copy)
            return rule_id

    def clear_faults(self) -> None:
        with self._lock:
            self._faults.clear()

    def get_requests(
        self,
        path: str | None = None,
        method: str | None = None,
        path_regex: str | None = None,
        include_mock: bool = False,
    ) -> list[dict[str, Any]]:
        with self._lock:
            out: list[dict[str, Any]] = []
            for req in self._requests:
                if not include_mock and req["path"].startswith("/__mock/"):
                    continue
                if method is not None and req["method"].upper() != method.upper():
                    continue
                if path is not None and not (
                    req["path"] == path or req["path"].startswith(path)
                ):
                    continue
                if path_regex is not None and not re.search(path_regex, req["path"]):
                    continue
                out.append(copy.deepcopy(req))
            return out

    def _begin_request(
        self,
        *,
        method: str,
        path: str,
        query: str,
        headers: dict[str, str],
        raw_body: str,
        body_json: Any,
    ) -> tuple[dict[str, Any], dict[str, Any] | None]:
        with self._lock:
            self._seq += 1
            matched_fault: dict[str, Any] | None = None
            for idx, rule in enumerate(self._faults):
                count = rule.get("count")
                if count is not None and count <= 0:
                    continue
                rule_method = rule.get("method")
                if rule_method is not None and rule_method.upper() != method.upper():
                    continue
                if re.search(rule["route"], path):
                    matched_fault = copy.deepcopy(rule)
                    if count is not None:
                        rule["count"] = count - 1
                        if rule["count"] <= 0:
                            self._faults.pop(idx)
                    break

            entry: dict[str, Any] = {
                "seq": self._seq,
                "timestamp": time.time(),
                "method": method,
                "path": path,
                "query": query,
                "headers": headers,
                "raw_body": raw_body,
                "body_json": copy.deepcopy(body_json),
                "validation_error": None,
                "fault_applied": matched_fault["rule_id"] if matched_fault else None,
                "response_status": None,
            }
            self._requests.append(entry)
            return entry, matched_fault

    def _finish_request(
        self,
        entry: dict[str, Any],
        *,
        response_status: int | None,
        validation_error: str | None = None,
    ) -> None:
        with self._lock:
            entry["response_status"] = response_status
            if validation_error is not None:
                entry["validation_error"] = validation_error

    def _handle_mock_control(
        self,
        method: str,
        path: str,
        query: str,
        body_json: Any,
        json_parse_error: str | None,
    ) -> tuple[int, dict[str, Any]]:
        if path == "/__mock/reset" and method == "POST":
            self.reset()
            return 200, {"success": True}

        if path == "/__mock/state":
            if method == "GET":
                return 200, {"success": True, "state": self.get_state()}
            if method == "POST":
                if json_parse_error is not None or not isinstance(body_json, dict):
                    return 400, {"success": False, "message": "Expected JSON object patch"}
                updated = self.set_state(body_json)
                return 200, {"success": True, "state": updated}

        if path == "/__mock/fault":
            if method == "POST":
                if json_parse_error is not None or not isinstance(body_json, dict):
                    return 400, {"success": False, "message": "Expected fault rule JSON object"}
                try:
                    rule_id = self.inject_fault(body_json)
                except ValueError as exc:
                    return 400, {"success": False, "message": str(exc)}
                return 200, {"success": True, "rule_id": rule_id}
            if method == "DELETE":
                self.clear_faults()
                return 200, {"success": True}

        if path == "/__mock/requests" and method == "GET":
            params = urllib.parse.parse_qs(query)
            req_path = params.get("path", [None])[0]
            req_method = params.get("method", [None])[0]
            include_mock = params.get("include_mock", ["0"])[0] in ("1", "true", "True")
            return 200, {
                "success": True,
                "requests": self.get_requests(
                    path=req_path, method=req_method, include_mock=include_mock
                ),
            }

        return 404, {"success": False, "message": f"Unknown mock control route {method} {path}"}

    def _check_bearer_auth(self, headers: dict[str, str]) -> int | None:
        auth = headers.get("authorization", "")
        if not auth.startswith("Bearer "):
            return None
        token = auth[len("Bearer "):].strip()
        if not token:
            return None
        return self._active_tokens.get(token)

    def _validation_fail(self, reason: str) -> tuple[int, str, bytes, str]:
        payload = {
            "success": False,
            "message": f"Validation error: {reason}",
            "validation_error": reason,
        }
        return 400, "application/json", json.dumps(payload).encode("utf-8"), reason

    def _json_ok(self, payload: Any, status: int = 200) -> tuple[int, str, bytes, None]:
        return status, "application/json", json.dumps(payload).encode("utf-8"), None

    def _handle_app_route(
        self,
        *,
        method: str,
        path: str,
        query: str,
        headers: dict[str, str],
        raw_body: str,
        body_json: Any,
        json_parse_error: str | None,
    ) -> tuple[int, str, bytes, str | None]:
        with self._lock:
            # 17. GET /version.json
            if method == "GET" and path == "/version.json":
                return self._json_ok(copy.deepcopy(self._state["version_manifest"]))

            # 18. GET /s2/favicons
            if method == "GET" and path == "/s2/favicons":
                return 200, "image/png", _FAVICON_PNG_BYTES, None

            # 1. POST /api/login (no auth required)
            if method == "POST" and path == "/api/login":
                if self.strict_schemas:
                    if json_parse_error is not None or not isinstance(body_json, dict):
                        return self._validation_fail("login body must be a JSON object")
                    if not _is_non_empty_str(body_json.get("email")):
                        return self._validation_fail("login requires non-empty string 'email'")
                    if not _is_non_empty_str(body_json.get("password")):
                        return self._validation_fail("login requires non-empty string 'password'")

                email = (body_json or {}).get("email", "")
                password = (body_json or {}).get("password", "")

                matched_user = None
                for user in self._state.get("users", []):
                    if user.get("email") == email or user.get("name") == email:
                        matched_user = user
                        break

                if matched_user is None or matched_user.get("password") != password:
                    return self._json_ok(
                        {"success": False, "message": "Username atau password salah."},
                        status=401,
                    )

                blocked_ip = matched_user.get("blocked_ip")
                if not blocked_ip and email in self._state.get("blocked_ips", []):
                    blocked_ip = "203.0.113.42"
                if blocked_ip:
                    return self._json_ok(
                        {
                            "success": False,
                            "message": "Login not allowed from this IP address",
                            "your_ip": blocked_ip,
                        },
                        status=200,
                    )

                token = matched_user.get("token") or f"mock-token-user-{matched_user['id']}"
                self._active_tokens[token] = int(matched_user["id"])
                user_payload = {
                    "id": matched_user["id"],
                    "name": matched_user["name"],
                    "email": matched_user["email"],
                    "role": copy.deepcopy(matched_user.get("role", {})),
                    "department": copy.deepcopy(matched_user.get("department", {})),
                }
                return self._json_ok(
                    {"success": True, "token": token, "user": user_payload}
                )

            # Endpoints #2-#16 require Bearer token when enforce_auth is True.
            auth_user_id: int | None = None
            if path.startswith("/api/"):
                if self._state.get("enforce_auth", True):
                    auth_user_id = self._check_bearer_auth(headers)
                    if auth_user_id is None:
                        return self._json_ok(
                            {"success": False, "message": "Host requires authentication"},
                            status=401,
                        )
            else:
                return self._json_ok(
                    {"success": False, "message": f"Not found: {method} {path}"},
                    status=404,
                )

            # 2. POST /api/logout
            if method == "POST" and path == "/api/logout":
                if self.strict_schemas and raw_body.strip():
                    if json_parse_error is not None or not isinstance(body_json, dict):
                        return self._validation_fail("logout body must be a JSON object")
                auth = headers.get("authorization", "")
                if auth.startswith("Bearer "):
                    tok = auth[len("Bearer "):].strip()
                    self._active_tokens.pop(tok, None)
                return self._json_ok({"success": True})

            # 3. POST /api/send-alasan
            if method == "POST" and path == "/api/send-alasan":
                if self.strict_schemas:
                    if json_parse_error is not None or not isinstance(body_json, dict):
                        return self._validation_fail("send-alasan body must be a JSON object")
                    if not _is_non_empty_str(body_json.get("email")):
                        return self._validation_fail("send-alasan requires non-empty string 'email'")
                    if not _is_non_empty_str(body_json.get("alasan")):
                        return self._validation_fail("send-alasan requires non-empty string 'alasan'")
                self._state["early_leave_reasons"].append(copy.deepcopy(body_json))
                return self._json_ok({"success": True})

            # 4. GET /api/get-time-at-work/{userId}
            m = re.fullmatch(r"/api/get-time-at-work/(\d+)", path)
            if method == "GET" and m:
                uid = m.group(1)
                by_user = self._state.get("work_time_by_user", {})
                raw_val = by_user.get(uid, self._state.get("work_time_raw", 0))
                return self._json_ok({"success": True, "data": {"raw": int(raw_val)}})

            # 5. POST /api/send-time-at-work
            if method == "POST" and path == "/api/send-time-at-work":
                if self.strict_schemas:
                    if json_parse_error is not None or not isinstance(body_json, dict):
                        return self._validation_fail("send-time-at-work body must be a JSON object")
                    if not _is_int(body_json.get("user_id")):
                        return self._validation_fail("send-time-at-work requires integer 'user_id'")
                    taw = body_json.get("time_at_work")
                    if not _is_int(taw) or taw < 0:
                        return self._validation_fail("send-time-at-work requires non-negative integer 'time_at_work'")
                uid = str(body_json["user_id"])
                self._state["work_time_by_user"][uid] = int(body_json["time_at_work"])
                return self._json_ok({"success": True})

            # 6. GET /api/get-check-in/{userId}
            m = re.fullmatch(r"/api/get-check-in/(\d+)", path)
            if method == "GET" and m:
                chk = self._state.get("check_in", {})
                return self._json_ok(
                    {
                        "success": True,
                        "clock_in": chk.get("clock_in"),
                        "clock_out": chk.get("clock_out"),
                    }
                )

            # 7. POST /api/send-productive-time
            if method == "POST" and path == "/api/send-productive-time":
                if self.strict_schemas:
                    if json_parse_error is not None or not isinstance(body_json, dict):
                        return self._validation_fail("send-productive-time body must be a JSON object")
                    if not _is_int(body_json.get("user_id")):
                        return self._validation_fail("send-productive-time requires integer 'user_id'")
                    pt = body_json.get("productive_time")
                    if not _is_int(pt) or pt < 0:
                        return self._validation_fail("send-productive-time requires non-negative integer 'productive_time'")
                return self._json_ok({"success": True})

            # 8. POST /api/productivity-app
            if method == "POST" and path == "/api/productivity-app":
                if self.strict_schemas:
                    if json_parse_error is not None or not isinstance(body_json, dict):
                        return self._validation_fail("productivity-app body must be a JSON object")
                    data = body_json.get("data")
                    if not isinstance(data, list) or len(data) == 0:
                        return self._validation_fail("productivity-app requires non-empty list 'data'")
                    for idx, item in enumerate(data):
                        if not isinstance(item, dict):
                            return self._validation_fail(f"productivity-app data[{idx}] must be an object")
                        if not _is_int(item.get("user_id")):
                            return self._validation_fail(f"productivity-app data[{idx}] requires integer 'user_id'")
                        if not _is_non_empty_str(item.get("app_name")):
                            return self._validation_fail(f"productivity-app data[{idx}] requires non-empty string 'app_name'")
                        dur = item.get("duration")
                        if not _is_number(dur) or dur <= 0:
                            return self._validation_fail(f"productivity-app data[{idx}] requires positive number 'duration'")
                        url_val = item.get("url")
                        if url_val is not None and not isinstance(url_val, str):
                            return self._validation_fail(f"productivity-app data[{idx}] 'url' must be str or null")
                        if item.get("status") not in ("productive", "non-productive", "neutral"):
                            return self._validation_fail(f"productivity-app data[{idx}] invalid 'status'")
                self._state["usage_reports"].append(copy.deepcopy(body_json))
                return self._json_ok({"success": True})

            # 9. POST /api/app-request/store
            if method == "POST" and path == "/api/app-request/store":
                if self.strict_schemas:
                    if json_parse_error is not None or not isinstance(body_json, dict):
                        return self._validation_fail("app-request/store body must be a JSON object")
                    if not _is_non_empty_str(body_json.get("application_name")):
                        return self._validation_fail("app-request/store requires non-empty string 'application_name'")
                    if body_json.get("productivity_status") not in ("productive", "non-productive", "neutral"):
                        return self._validation_fail("app-request/store invalid 'productivity_status'")
                    if not _is_int(body_json.get("user_id")):
                        return self._validation_fail("app-request/store requires integer 'user_id'")
                    if "process_name" in body_json and body_json["process_name"] is not None and not isinstance(body_json["process_name"], str):
                        return self._validation_fail("app-request/store 'process_name' must be string")
                    if "url" in body_json and body_json["url"] is not None and not isinstance(body_json["url"], str):
                        return self._validation_fail("app-request/store 'url' must be string")
                self._state["submitted_app_requests"].append(copy.deepcopy(body_json))
                return self._json_ok({"success": True})

            # 10. GET /api/app-request/all
            if method == "GET" and path == "/api/app-request/all":
                return self._json_ok(
                    {"success": True, "data": copy.deepcopy(self._state.get("apps", []))}
                )

            # 11. GET /api/task-by-user/{userId}
            m = re.fullmatch(r"/api/task-by-user/(\d+)", path)
            if method == "GET" and m:
                uid = int(m.group(1))
                tasks = [
                    copy.deepcopy(t)
                    for t in self._state.get("tasks", [])
                    if int(t.get("user_id", -1)) == uid
                ]
                return self._json_ok({"success": True, "data": tasks})

            # 12. GET /api/get-current-task-status/{taskId}
            m = re.fullmatch(r"/api/get-current-task-status/(\d+)", path)
            if method == "GET" and m:
                tid = m.group(1)
                status_val = self._state.get("task_statuses", {}).get(tid, "pending")
                return self._json_ok({"success": True, "data": copy.deepcopy(status_val)})

            # 13. PUT /api/end-implementation/{taskId}
            m = re.fullmatch(r"/api/end-implementation/(\d+)", path)
            if method == "PUT" and m:
                if self.strict_schemas:
                    if json_parse_error is not None or not isinstance(body_json, dict):
                        return self._validation_fail("end-implementation body must be a JSON object")
                    if body_json.get("status") != "stop":
                        return self._validation_fail("end-implementation requires status == 'stop'")
                return self._json_ok({"success": True})

            # 14. POST /api/ping
            if method == "POST" and path == "/api/ping":
                if self.strict_schemas:
                    if json_parse_error is not None or not isinstance(body_json, dict) or not body_json:
                        return self._validation_fail("ping requires non-empty JSON object")
                    has_task_id = "task_id" in body_json
                    has_user_id = "user_id" in body_json
                    if not has_task_id and not has_user_id:
                        return self._validation_fail("ping requires 'task_id' (str) or 'user_id' (int)")
                    if has_task_id and not _is_non_empty_str(body_json.get("task_id")):
                        return self._validation_fail("ping 'task_id' must be a non-empty string")
                    if has_user_id and not _is_int(body_json.get("user_id")):
                        return self._validation_fail("ping 'user_id' must be an integer")
                return self._json_ok({"success": True})

            # 15. POST /api/send-detail-pekerjaan
            if method == "POST" and path == "/api/send-detail-pekerjaan":
                if self.strict_schemas:
                    if json_parse_error is not None or not isinstance(body_json, dict):
                        return self._validation_fail("send-detail-pekerjaan body must be a JSON object")
                    if not _is_int(body_json.get("task_id")):
                        return self._validation_fail("send-detail-pekerjaan requires integer 'task_id'")
                    if not _is_int(body_json.get("user_id")):
                        return self._validation_fail("send-detail-pekerjaan requires integer 'user_id'")
                    if not isinstance(body_json.get("message"), str):
                        return self._validation_fail("send-detail-pekerjaan requires string 'message'")
                self._state["task_details"].append(copy.deepcopy(body_json))
                return self._json_ok({"success": True})

            # 16. POST /api/update-status-task/{taskId}/{userId}
            m = re.fullmatch(r"/api/update-status-task/(\d+)/(\d+)", path)
            if method == "POST" and m:
                tid = m.group(1)
                if self.strict_schemas:
                    if json_parse_error is not None or not isinstance(body_json, dict):
                        return self._validation_fail("update-status-task body must be a JSON object")
                    if body_json.get("status") != "need-review":
                        return self._validation_fail("update-status-task requires status == 'need-review'")
                    alasan = body_json.get("alasan")
                    if (
                        not isinstance(alasan, str)
                        or len(alasan.strip()) < 1
                        or len(alasan) > 500
                    ):
                        return self._validation_fail(
                            "update-status-task requires 'alasan' (non-empty and <= 500 chars)"
                        )
                self._state["task_statuses"][tid] = "need-review"
                for task in self._state.get("tasks", []):
                    if str(task.get("id")) == tid:
                        task["status"] = "need-review"
                return self._json_ok({"success": True})

            return self._json_ok(
                {"success": False, "message": f"Unknown API endpoint: {method} {path}"},
                status=404,
            )
