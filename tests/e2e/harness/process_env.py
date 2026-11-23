"""Isolated process, build, and environment harness for Deskmon E2E tests."""

from __future__ import annotations

import atexit
import csv
import ctypes
from ctypes import wintypes
import hashlib
import io
import os
import re
import shutil
import subprocess
import sys
import tempfile
import threading
import time
import uuid
from dataclasses import dataclass
from pathlib import Path
from typing import IO, Any

DEFAULT_REF_BIN = Path(
    r"C:\Users\Pranala Istow\Documents\Deskmon 1.0.3.3\Deskmon\build\Desktop_Qt_6_9_0_MinGW_64_bit-Release\Deskmon.exe"
)

_BUILD_LOCK = threading.Lock()
_ACTIVE_PIDS_LOCK = threading.Lock()
_ACTIVE_PIDS: set[int] = set()


def _cleanup_leaked_pids() -> None:
    with _ACTIVE_PIDS_LOCK:
        pids = list(_ACTIVE_PIDS)
    for pid in pids:
        try:
            subprocess.run(
                ["taskkill", "/PID", str(pid), "/T", "/F"],
                capture_output=True,
                text=True,
                timeout=5,
                check=False,
            )
        except Exception:
            pass
        with _ACTIVE_PIDS_LOCK:
            _ACTIVE_PIDS.discard(pid)


atexit.register(_cleanup_leaked_pids)


_JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE = 0x00002000
_JOB_OBJECT_EXTENDED_LIMIT_INFO_CLASS = 9


class _JOBOBJECT_BASIC_LIMIT_INFORMATION(ctypes.Structure):
    _fields_ = [
        ("PerProcessUserTimeLimit", wintypes.LARGE_INTEGER),
        ("PerJobUserTimeLimit", wintypes.LARGE_INTEGER),
        ("LimitFlags", wintypes.DWORD),
        ("MinimumWorkingSetSize", ctypes.c_size_t),
        ("MaximumWorkingSetSize", ctypes.c_size_t),
        ("ActiveProcessLimit", wintypes.DWORD),
        ("Affinity", ctypes.c_size_t),
        ("PriorityClass", wintypes.DWORD),
        ("SchedulingClass", wintypes.DWORD),
    ]


class _IO_COUNTERS(ctypes.Structure):
    _fields_ = [
        ("ReadOperationCount", ctypes.c_uint64),
        ("WriteOperationCount", ctypes.c_uint64),
        ("OtherOperationCount", ctypes.c_uint64),
        ("ReadTransferCount", ctypes.c_uint64),
        ("WriteTransferCount", ctypes.c_uint64),
        ("OtherTransferCount", ctypes.c_uint64),
    ]


class _JOBOBJECT_EXTENDED_LIMIT_INFORMATION(ctypes.Structure):
    _fields_ = [
        ("BasicLimitInformation", _JOBOBJECT_BASIC_LIMIT_INFORMATION),
        ("IoInfo", _IO_COUNTERS),
        ("ProcessMemoryLimit", ctypes.c_size_t),
        ("JobMemoryLimit", ctypes.c_size_t),
        ("PeakProcessMemoryUsed", ctypes.c_size_t),
        ("PeakJobMemoryUsed", ctypes.c_size_t),
    ]


def _create_kill_on_close_job() -> int | None:
    if os.name != "nt":
        return None
    try:
        kernel32 = ctypes.windll.kernel32  # type: ignore[attr-defined]
        kernel32.CreateJobObjectW.argtypes = [ctypes.c_void_p, ctypes.c_wchar_p]
        kernel32.CreateJobObjectW.restype = ctypes.c_void_p
        kernel32.SetInformationJobObject.argtypes = [
            ctypes.c_void_p,
            ctypes.c_int,
            ctypes.c_void_p,
            wintypes.DWORD,
        ]
        kernel32.SetInformationJobObject.restype = wintypes.BOOL
        kernel32.CloseHandle.argtypes = [ctypes.c_void_p]
        kernel32.CloseHandle.restype = wintypes.BOOL

        job = kernel32.CreateJobObjectW(None, None)
        if not job:
            return None
        info = _JOBOBJECT_EXTENDED_LIMIT_INFORMATION()
        info.BasicLimitInformation.LimitFlags = _JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
        ok = kernel32.SetInformationJobObject(
            job,
            _JOB_OBJECT_EXTENDED_LIMIT_INFO_CLASS,
            ctypes.byref(info),
            ctypes.sizeof(info),
        )
        if not ok:
            kernel32.CloseHandle(job)
            return None
        return int(job)
    except Exception:
        return None


def _assign_process_to_job(job_handle: int | None, process_handle: Any) -> bool:
    if os.name != "nt" or not job_handle or not process_handle:
        return False
    try:
        kernel32 = ctypes.windll.kernel32  # type: ignore[attr-defined]
        kernel32.AssignProcessToJobObject.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
        kernel32.AssignProcessToJobObject.restype = wintypes.BOOL
        return bool(kernel32.AssignProcessToJobObject(job_handle, int(process_handle)))
    except Exception:
        return False


def _terminate_and_close_job(job_handle: int | None) -> None:
    if os.name != "nt" or not job_handle:
        return
    try:
        kernel32 = ctypes.windll.kernel32  # type: ignore[attr-defined]
        kernel32.TerminateJobObject.argtypes = [ctypes.c_void_p, wintypes.UINT]
        kernel32.TerminateJobObject.restype = wintypes.BOOL
        kernel32.CloseHandle.argtypes = [ctypes.c_void_p]
        kernel32.CloseHandle.restype = wintypes.BOOL
        kernel32.TerminateJobObject(job_handle, 1)
        kernel32.CloseHandle(job_handle)
    except Exception:
        pass


@dataclass(frozen=True)
class QtToolchain:
    qt_dir: Path
    qt_bin: Path
    qt_plugins: Path
    cmake_exe: Path
    ninja_exe: Path
    mingw_bin: Path


def find_qt_toolchain(qt_dir: str | Path | None = None) -> QtToolchain:
    """Locate Qt 6.11.2 MinGW + CMake + Ninja + MinGW toolchain."""
    searched: list[str] = []
    candidates: list[Path] = []
    if qt_dir is not None:
        candidates.append(Path(qt_dir))
    env_qt = os.environ.get("DESKMON_QT_DIR", "").strip()
    if env_qt:
        candidates.append(Path(env_qt))
    candidates.extend(
        [
            Path(r"C:\Qt\6.11.2\mingw_64"),
            Path(r"C:\Qt6112\6.11.2\mingw_64"),
        ]
    )

    chosen_qt: Path | None = None
    for cand in candidates:
        searched.append(str(cand))
        if (cand / "bin" / "Qt6Core.dll").is_file() and (cand / "plugins").is_dir():
            chosen_qt = cand
            break

    if chosen_qt is None:
        raise FileNotFoundError(
            f"Qt 6.11.2 mingw_64 toolchain not found. Searched: {searched}"
        )

    tools_roots = [
        chosen_qt.parent.parent / "Tools",
        Path(r"C:\Qt\Tools"),
        Path(r"C:\Qt6112\Tools"),
    ]

    cmake_exe: Path | None = None
    ninja_exe: Path | None = None
    mingw_bin: Path | None = None

    for root in tools_roots:
        if cmake_exe is None:
            p = root / "CMake_64" / "bin" / "cmake.exe"
            searched.append(str(p))
            if p.is_file():
                cmake_exe = p
        if ninja_exe is None:
            p = root / "Ninja" / "ninja.exe"
            searched.append(str(p))
            if p.is_file():
                ninja_exe = p
        if mingw_bin is None:
            for mingw_name in ("mingw1310_64", "mingw1120_64"):
                p = root / mingw_name / "bin"
                searched.append(str(p))
                if (p / "g++.exe").is_file():
                    mingw_bin = p
                    break

    if cmake_exe is None and (which_cmake := shutil.which("cmake")):
        cmake_exe = Path(which_cmake)
    if ninja_exe is None and (which_ninja := shutil.which("ninja")):
        ninja_exe = Path(which_ninja)
    if mingw_bin is None and (which_gxx := shutil.which("g++")):
        mingw_bin = Path(which_gxx).parent

    missing = []
    if cmake_exe is None:
        missing.append("cmake.exe")
    if ninja_exe is None:
        missing.append("ninja.exe")
    if mingw_bin is None:
        missing.append("mingw g++.exe")
    if missing:
        raise FileNotFoundError(
            f"Missing build tools {missing}. Searched: {searched}"
        )

    return QtToolchain(
        qt_dir=chosen_qt,
        qt_bin=chosen_qt / "bin",
        qt_plugins=chosen_qt / "plugins",
        cmake_exe=cmake_exe,
        ninja_exe=ninja_exe,
        mingw_bin=mingw_bin,
    )


def ensure_e2e_build(
    repo_root: Path,
    build_dir: Path | None = None,
    qt_dir: str | Path | None = None,
    force_reconfigure: bool = False,
) -> Path:
    """Configure and build Deskmon.exe into build_e2e/ with -DCONSOLE_LOGS=ON."""
    repo_root = Path(repo_root).resolve()
    target_build_dir = Path(build_dir).resolve() if build_dir else (repo_root / "build_e2e")
    tc = find_qt_toolchain(qt_dir=qt_dir)

    env = os.environ.copy()
    python_dir = str(Path(sys.executable).resolve().parent)
    path_prefix = os.pathsep.join(
        [
            str(tc.qt_bin),
            str(tc.cmake_exe.parent),
            str(tc.mingw_bin),
            str(tc.ninja_exe.parent),
            python_dir,
        ]
    )
    env["PATH"] = f"{path_prefix}{os.pathsep}{env.get('PATH', '')}"

    with _BUILD_LOCK:
        cache_file = target_build_dir / "CMakeCache.txt"
        if force_reconfigure or not cache_file.is_file():
            cmd_cfg = [
                str(tc.cmake_exe),
                "-S",
                str(repo_root),
                "-B",
                str(target_build_dir),
                "-G",
                "Ninja",
                "-DCMAKE_BUILD_TYPE=Debug",
                "-DCONSOLE_LOGS=ON",
                f"-DCMAKE_PREFIX_PATH={tc.qt_dir}",
            ]
            res_cfg = subprocess.run(
                cmd_cfg,
                cwd=str(repo_root),
                env=env,
                capture_output=True,
                text=True,
                encoding="utf-8",
                errors="replace",
            )
            if res_cfg.returncode != 0:
                raise RuntimeError(
                    f"CMake configure failed ({res_cfg.returncode}):\n"
                    f"{res_cfg.stdout}\n{res_cfg.stderr}"
                )

        cmd_build = [
            str(tc.cmake_exe),
            "--build",
            str(target_build_dir),
            "--target",
            "Deskmon",
        ]
        res_build = subprocess.run(
            cmd_build,
            cwd=str(repo_root),
            env=env,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
        if res_build.returncode != 0:
            raise RuntimeError(
                f"CMake build failed ({res_build.returncode}):\n"
                f"{res_build.stdout}\n{res_build.stderr}"
            )

    exe_path = target_build_dir / "Deskmon.exe"
    if not exe_path.is_file():
        raise FileNotFoundError(f"Expected built binary missing at {exe_path}")
    return exe_path


def find_ref_binary(explicit_path: str | Path | None = None) -> Path | None:
    """Locate the prebuilt Deskmon 1.0.3.3 Release binary without failing on sandbox locks."""
    candidates: list[Path] = []
    if explicit_path is not None:
        candidates.append(Path(explicit_path))
    env_ref = os.environ.get("DESKMON_E2E_REF_BIN", "").strip()
    if env_ref:
        candidates.append(Path(env_ref))
    candidates.append(DEFAULT_REF_BIN)

    for cand in candidates:
        try:
            if cand.is_file() and os.access(cand, os.R_OK):
                return cand
        except (PermissionError, OSError):
            continue
    return None


def _qt_shared_memory_native_key(instance_key: str) -> str:
    """Compute Qt 6 QSharedMemoryPrivate::makePlatformSafeKey for Windows."""
    sanitized = re.sub(r"[^A-Za-z0-9]", "", instance_key)
    digest = hashlib.sha1(instance_key.encode("utf-8")).hexdigest()
    return f"qipc_sharedmemory_{sanitized}{digest}"


def preflight_check_host_instance(
    instance_key: str = "DeskmonAppInstance",
) -> tuple[bool, str]:
    """Detect whether a host Deskmon.exe or QSharedMemory lock is active.

    Does NOT kill or interfere with any pre-existing host process.
    """
    issues: list[str] = []

    try:
        res = subprocess.run(
            ["tasklist", "/FI", "IMAGENAME eq Deskmon.exe", "/FO", "CSV", "/NH"],
            capture_output=True,
            text=True,
            timeout=5,
            check=False,
        )
        with _ACTIVE_PIDS_LOCK:
            tracked = set(_ACTIVE_PIDS)
        external_pids: list[str] = []
        reader = csv.reader(io.StringIO(res.stdout))
        for row in reader:
            if len(row) >= 2 and row[0].lower() == "deskmon.exe":
                pid_str = row[1].strip()
                try:
                    if int(pid_str) not in tracked:
                        external_pids.append(pid_str)
                except ValueError:
                    external_pids.append(pid_str)
        if external_pids:
            issues.append(f"External Deskmon.exe process(es) active: PIDs {external_pids}")
    except Exception as exc:
        issues.append(f"tasklist check failed: {exc}")

    try:
        kernel32 = ctypes.windll.kernel32  # type: ignore[attr-defined]
        kernel32.OpenFileMappingW.argtypes = [ctypes.c_uint32, ctypes.c_bool, ctypes.c_wchar_p]
        kernel32.OpenFileMappingW.restype = ctypes.c_void_p
        kernel32.CloseHandle.argtypes = [ctypes.c_void_p]
        kernel32.CloseHandle.restype = ctypes.c_bool

        native_key = _qt_shared_memory_native_key(instance_key)
        for prefix in ("", "Local\\", "Global\\"):
            name = f"{prefix}{native_key}"
            handle = kernel32.OpenFileMappingW(0x0004, False, name)
            if handle:
                kernel32.CloseHandle(handle)
                issues.append(
                    f"Active QSharedMemory mapping detected for key={instance_key!r} ({name})"
                )
                break
    except Exception as exc:
        issues.append(f"OpenFileMappingW check failed: {exc}")

    if issues:
        return False, "; ".join(issues)
    return True, f"No host Deskmon.exe or QSharedMemory collision for key={instance_key!r}"


class ManagedProcess:
    """Subprocess wrapper with file-backed log capture and process-tree teardown."""

    def __init__(
        self,
        popen: subprocess.Popen[bytes] | subprocess.Popen[str],
        stdout_path: Path,
        stderr_path: Path,
        stdout_file: IO[str],
        stderr_file: IO[str],
        job_handle: int | None = None,
    ) -> None:
        self._proc = popen
        self.pid = popen.pid
        self.stdout_path = stdout_path
        self.stderr_path = stderr_path
        self._stdout_file = stdout_file
        self._stderr_file = stderr_file
        self._job_handle = job_handle
        self._closed = False
        with _ACTIVE_PIDS_LOCK:
            _ACTIVE_PIDS.add(self.pid)

    @property
    def returncode(self) -> int | None:
        return self._proc.poll()

    def is_running(self) -> bool:
        return self._proc.poll() is None

    def stdout_text(self) -> str:
        try:
            return self.stdout_path.read_text(encoding="utf-8", errors="replace")
        except OSError:
            return ""

    def stderr_text(self) -> str:
        try:
            return self.stderr_path.read_text(encoding="utf-8", errors="replace")
        except OSError:
            return ""

    def combined_logs(self) -> str:
        out = self.stdout_text()
        err = self.stderr_text()
        if out and err:
            return f"{out}\n{err}"
        return out or err

    def wait_for_log(
        self,
        pattern: str | re.Pattern[str],
        timeout: float = 10.0,
        poll_interval: float = 0.05,
    ) -> re.Match[str]:
        compiled = re.compile(pattern) if isinstance(pattern, str) else pattern
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            logs = self.combined_logs()
            m = compiled.search(logs)
            if m is not None:
                return m
            if not self.is_running():
                # Check once more after process exit in case final output flushed.
                logs = self.combined_logs()
                m = compiled.search(logs)
                if m is not None:
                    return m
                raise RuntimeError(
                    f"Process {self.pid} exited with code {self.returncode} "
                    f"before matching pattern {compiled.pattern!r}.\n"
                    f"--- Captured logs ---\n{logs}"
                )
            time.sleep(poll_interval)

        logs = self.combined_logs()
        raise TimeoutError(
            f"Timed out after {timeout:.1f}s waiting for log pattern "
            f"{compiled.pattern!r} from PID {self.pid}.\n"
            f"--- Captured logs ---\n{logs}"
        )

    def wait(self, timeout: float = 10.0) -> int:
        rc = self._proc.wait(timeout=timeout)
        self._close_handles()
        with _ACTIVE_PIDS_LOCK:
            _ACTIVE_PIDS.discard(self.pid)
        return rc

    def _close_handles(self) -> None:
        if self._closed:
            return
        self._closed = True
        for f in (self._stdout_file, self._stderr_file):
            try:
                f.flush()
            except Exception:
                pass
            try:
                f.close()
            except Exception:
                pass

    def terminate(self, timeout: float = 5.0) -> int | None:
        rc = self._proc.poll()
        if rc is None:
            if os.name == "nt":
                try:
                    subprocess.run(
                        ["taskkill", "/PID", str(self.pid), "/T", "/F"],
                        capture_output=True,
                        text=True,
                        timeout=5,
                        check=False,
                    )
                except Exception:
                    pass
                try:
                    rc = self._proc.wait(timeout=timeout)
                except subprocess.TimeoutExpired:
                    try:
                        self._proc.terminate()
                        rc = self._proc.wait(timeout=3.0)
                    except Exception:
                        rc = self._proc.poll()
            else:
                try:
                    self._proc.terminate()
                    rc = self._proc.wait(timeout=timeout)
                except subprocess.TimeoutExpired:
                    self._proc.kill()
                    rc = self._proc.wait(timeout=3.0)

        if self._job_handle is not None:
            _terminate_and_close_job(self._job_handle)
            self._job_handle = None

        self._close_handles()
        with _ACTIVE_PIDS_LOCK:
            _ACTIVE_PIDS.discard(self.pid)
        return rc


class IsolatedDeskmonEnv:
    """Context manager providing an isolated temp workspace, env, and process cleanup."""

    def __init__(
        self,
        api_base_url: str | None = None,
        update_url: str | None = None,
        instance_key: str | None = None,
        use_db_path: bool = True,
        qt_qpa_platform: str | None = "offscreen",
        qt_dir: str | Path | None = None,
        extra_env: dict[str, str] | None = None,
        keep_temp: bool = False,
    ) -> None:
        self.api_base_url = api_base_url
        self.update_url = update_url
        self.instance_key = (
            f"DeskmonE2E_{uuid.uuid4().hex[:12]}"
            if instance_key is None
            else instance_key
        )
        self.use_db_path = use_db_path
        self.qt_qpa_platform = qt_qpa_platform
        self.qt_dir = qt_dir
        self.extra_env = dict(extra_env or {})
        self.keep_temp = keep_temp

        self.root = Path(tempfile.mkdtemp(prefix="deskmon_e2e_"))
        self.cwd = self.root / "cwd"
        self.localappdata = self.root / "localappdata"
        self.appdata = self.root / "appdata"
        self.db_dir = self.root / "db"
        self.db_path = self.db_dir / "deskmon.db"
        self.logs_dir = self.root / "logs"

        for d in (self.cwd, self.localappdata, self.appdata, self.db_dir, self.logs_dir):
            d.mkdir(parents=True, exist_ok=True)

        self._processes: list[ManagedProcess] = []
        self._proc_counter = 0

    def __enter__(self) -> "IsolatedDeskmonEnv":
        return self

    def __exit__(self, exc_type: Any, exc: Any, tb: Any) -> None:
        self.cleanup()

    def cleanup(self) -> None:
        for proc in reversed(self._processes):
            try:
                proc.terminate()
            except Exception:
                pass
        self._processes.clear()
        if not self.keep_temp:
            shutil.rmtree(self.root, ignore_errors=True)

    def build_env(
        self,
        is_ref: bool = False,
        env_overrides: dict[str, str | None] | None = None,
    ) -> dict[str, str]:
        env = os.environ.copy()

        if not is_ref:
            try:
                tc = find_qt_toolchain(qt_dir=self.qt_dir)
                prefix = f"{tc.qt_bin}{os.pathsep}{tc.mingw_bin}"
                env["PATH"] = f"{prefix}{os.pathsep}{env.get('PATH', '')}"
                env["QT_PLUGIN_PATH"] = str(tc.qt_plugins)
            except FileNotFoundError:
                pass
        else:
            # Ensure 1.0.3.3 Release uses its own adjacent Qt 6.9.0 DLLs/plugins cleanly.
            env.pop("QT_PLUGIN_PATH", None)

        env["LOCALAPPDATA"] = str(self.localappdata)
        env["APPDATA"] = str(self.appdata)

        if self.use_db_path:
            env["DESKMON_DB_PATH"] = str(self.db_path)
        else:
            env.pop("DESKMON_DB_PATH", None)

        if self.api_base_url is not None:
            env["DESKMON_API_BASE_URL"] = self.api_base_url
        else:
            env.pop("DESKMON_API_BASE_URL", None)

        if self.update_url is not None:
            env["DESKMON_UPDATE_URL"] = self.update_url
        else:
            env.pop("DESKMON_UPDATE_URL", None)

        if self.instance_key == "":
            env.pop("DESKMON_INSTANCE_KEY", None)
        else:
            env["DESKMON_INSTANCE_KEY"] = self.instance_key

        if self.qt_qpa_platform is not None:
            env["QT_QPA_PLATFORM"] = self.qt_qpa_platform
        else:
            env.pop("QT_QPA_PLATFORM", None)

        for k, v in self.extra_env.items():
            env[k] = v

        if env_overrides:
            for k, v in env_overrides.items():
                if v is None:
                    env.pop(k, None)
                else:
                    env[k] = v

        return env

    def launch(
        self,
        exe_path: str | Path,
        args: list[str] | None = None,
        env_overrides: dict[str, str | None] | None = None,
        cwd: Path | None = None,
        is_ref: bool | None = None,
    ) -> ManagedProcess:
        exe = Path(exe_path)
        if is_ref is None:
            is_ref = "Deskmon 1.0.3.3" in str(exe)

        merged_env = self.build_env(is_ref=is_ref, env_overrides=env_overrides)
        self._proc_counter += 1
        idx = self._proc_counter
        stdout_path = self.logs_dir / f"proc_{idx}.stdout.log"
        stderr_path = self.logs_dir / f"proc_{idx}.stderr.log"

        stdout_f = open(stdout_path, "w", encoding="utf-8", errors="replace", buffering=1)
        stderr_f = open(stderr_path, "w", encoding="utf-8", errors="replace", buffering=1)

        job_handle = _create_kill_on_close_job()
        try:
            popen = subprocess.Popen(
                [str(exe), *(args or [])],
                cwd=str(cwd or self.cwd),
                env=merged_env,
                stdout=stdout_f,
                stderr=stderr_f,
                text=True,
                encoding="utf-8",
                errors="replace",
            )
            _assign_process_to_job(job_handle, getattr(popen, "_handle", None))
        except Exception:
            _terminate_and_close_job(job_handle)
            stdout_f.close()
            stderr_f.close()
            raise

        managed = ManagedProcess(
            popen,
            stdout_path,
            stderr_path,
            stdout_f,
            stderr_f,
            job_handle=job_handle,
        )
        self._processes.append(managed)
        return managed
