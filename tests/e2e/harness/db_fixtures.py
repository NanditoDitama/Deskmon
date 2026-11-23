"""SQLite fixture seeders, read-only snapshot readers, and parity assertions.

Covers both Deskmon 1.0.3.3 dual databases (``activity_logs.db`` +
``produktif_app_db.db``) and DeskmonV2's unified ``deskmon.db`` schema
(8 tables + 4 legacy compatibility views ``log``, ``aplikasi``, ``task``,
``work_time``).
"""

from __future__ import annotations

import sqlite3
from pathlib import Path
from typing import Any

LEGACY_ACTIVITY_DDL = """
PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT UNIQUE NOT NULL,
    password TEXT NOT NULL,
    department TEXT,
    profile_image TEXT,
    email TEXT,
    role TEXT,
    token TEXT
);

CREATE TABLE IF NOT EXISTS log (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    id_user INTEGER NOT NULL,
    start_time INTEGER NOT NULL,
    end_time INTEGER NOT NULL,
    app_name TEXT,
    title TEXT,
    url TEXT,
    FOREIGN KEY(id_user) REFERENCES users(id) ON DELETE CASCADE
);
"""

LEGACY_PRODUCTIVITY_DDL = """
CREATE TABLE IF NOT EXISTS aplikasi (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    aplikasi TEXT NOT NULL,
    window_title TEXT,
    url TEXT,
    jenis INTEGER NOT NULL,
    productivity INTEGER NOT NULL DEFAULT 0,
    for_user TEXT NOT NULL DEFAULT '0'
);

CREATE TABLE IF NOT EXISTS task (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    project_name TEXT NOT NULL,
    task TEXT,
    max_time INTEGER NOT NULL,
    time_usage INTEGER NOT NULL,
    active BOOLEAN NOT NULL,
    status TEXT NOT NULL,
    paused BOOLEAN NOT NULL DEFAULT 0,
    user_id INTEGER NOT NULL,
    created_at TEXT
);

CREATE TABLE IF NOT EXISTS completed_tasks (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    project_name TEXT,
    task TEXT,
    max_time INTEGER,
    time_usage INTEGER,
    completed_time INTEGER,
    user_id INTEGER
);

CREATE TABLE IF NOT EXISTS idle_settings (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    threshold_seconds INTEGER
);

CREATE TABLE IF NOT EXISTS log_paused (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    task_id INTEGER NOT NULL,
    start_reality TEXT NOT NULL,
    end_reality TEXT,
    current_status TEXT NOT NULL,
    FOREIGN KEY(task_id) REFERENCES task(id)
);

CREATE TABLE IF NOT EXISTS work_time (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    date TEXT NOT NULL,
    elapsed_seconds INTEGER NOT NULL DEFAULT 0,
    UNIQUE(user_id, date)
);
"""

V2_UNIFIED_DDL = """
PRAGMA journal_mode = WAL;
PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT UNIQUE NOT NULL,
    password TEXT NOT NULL,
    email TEXT,
    department TEXT,
    role TEXT,
    token TEXT,
    profile_image TEXT
);

CREATE TABLE IF NOT EXISTS activity_logs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    start_time INTEGER NOT NULL,
    end_time INTEGER NOT NULL,
    app_name TEXT,
    title TEXT,
    url TEXT,
    FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS productivity_apps (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    app_name TEXT NOT NULL,
    window_title TEXT,
    url TEXT,
    productivity_type INTEGER NOT NULL,
    productivity INTEGER NOT NULL DEFAULT 0,
    for_user TEXT NOT NULL DEFAULT '0'
);

CREATE TABLE IF NOT EXISTS idle_settings (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    threshold_seconds INTEGER NOT NULL DEFAULT 180
);

CREATE TABLE IF NOT EXISTS tasks (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    project_name TEXT NOT NULL,
    task_desc TEXT,
    max_time INTEGER NOT NULL DEFAULT 0,
    time_usage INTEGER NOT NULL DEFAULT 0,
    active BOOLEAN NOT NULL DEFAULT 0,
    status TEXT NOT NULL DEFAULT 'pending',
    paused BOOLEAN NOT NULL DEFAULT 0,
    created_at TEXT
);

CREATE TABLE IF NOT EXISTS completed_tasks (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    project_name TEXT,
    task_desc TEXT,
    max_time INTEGER,
    time_usage INTEGER,
    completed_time INTEGER
);

CREATE TABLE IF NOT EXISTS task_pause_logs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    task_id INTEGER NOT NULL,
    paused_at TEXT NOT NULL,
    resumed_at TEXT,
    status TEXT NOT NULL,
    FOREIGN KEY(task_id) REFERENCES tasks(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS work_time_records (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    work_date TEXT NOT NULL,
    elapsed_seconds INTEGER NOT NULL DEFAULT 0,
    UNIQUE(user_id, work_date)
);

CREATE VIEW IF NOT EXISTS log AS
SELECT id, user_id AS id_user, start_time, end_time, app_name, title, url
FROM activity_logs;

CREATE VIEW IF NOT EXISTS aplikasi AS
SELECT id, app_name AS aplikasi, window_title, url, productivity_type AS jenis, productivity, for_user
FROM productivity_apps;

CREATE VIEW IF NOT EXISTS task AS
SELECT id, project_name, task_desc AS task, max_time, time_usage, active, status, paused, user_id, created_at
FROM tasks;

CREATE VIEW IF NOT EXISTS work_time AS
SELECT id, user_id, work_date AS date, elapsed_seconds
FROM work_time_records;
"""

V2_EXPECTED_TABLES = {
    "users",
    "activity_logs",
    "productivity_apps",
    "idle_settings",
    "tasks",
    "completed_tasks",
    "task_pause_logs",
    "work_time_records",
}

V2_EXPECTED_VIEWS = {"log", "aplikasi", "task", "work_time"}


def _connect_rw(db_path: str | Path) -> sqlite3.Connection:
    p = Path(db_path)
    p.parent.mkdir(parents=True, exist_ok=True)
    conn = sqlite3.connect(str(p))
    conn.row_factory = sqlite3.Row
    return conn


def _connect_ro(db_path: str | Path) -> sqlite3.Connection:
    p = Path(db_path).resolve()
    if not p.is_file():
        raise FileNotFoundError(f"SQLite database not found: {p}")
    uri = f"{p.as_uri()}?mode=ro"
    conn = sqlite3.connect(uri, uri=True)
    conn.row_factory = sqlite3.Row
    return conn


def _pick(row: dict[str, Any], *keys: str, default: Any = None) -> Any:
    for k in keys:
        if k in row:
            return row[k]
    return default


def seed_1033_dbs(
    activity_db_path: str | Path,
    productivity_db_path: str | Path,
    data: dict[str, Any] | None = None,
) -> None:
    """Create and seed Deskmon 1.0.3.3 dual SQLite databases."""
    dataset = data or {}
    act_conn = _connect_rw(activity_db_path)
    prod_conn = _connect_rw(productivity_db_path)
    try:
        act_conn.executescript(LEGACY_ACTIVITY_DDL)
        prod_conn.executescript(LEGACY_PRODUCTIVITY_DDL)

        for u in dataset.get("users", []):
            if "id" in u:
                act_conn.execute(
                    """
                    INSERT INTO users (id, username, password, department, profile_image, email, role, token)
                    VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                    """,
                    (
                        u["id"],
                        u["username"],
                        u.get("password", ""),
                        u.get("department"),
                        u.get("profile_image"),
                        u.get("email"),
                        u.get("role"),
                        u.get("token"),
                    ),
                )
            else:
                act_conn.execute(
                    """
                    INSERT INTO users (username, password, department, profile_image, email, role, token)
                    VALUES (?, ?, ?, ?, ?, ?, ?)
                    """,
                    (
                        u["username"],
                        u.get("password", ""),
                        u.get("department"),
                        u.get("profile_image"),
                        u.get("email"),
                        u.get("role"),
                        u.get("token"),
                    ),
                )

        for row in dataset.get("logs", []):
            uid = _pick(row, "user_id", "id_user")
            if "id" in row:
                act_conn.execute(
                    """
                    INSERT INTO log (id, id_user, start_time, end_time, app_name, title, url)
                    VALUES (?, ?, ?, ?, ?, ?, ?)
                    """,
                    (
                        row["id"],
                        uid,
                        row["start_time"],
                        row["end_time"],
                        row.get("app_name"),
                        row.get("title"),
                        row.get("url"),
                    ),
                )
            else:
                act_conn.execute(
                    """
                    INSERT INTO log (id_user, start_time, end_time, app_name, title, url)
                    VALUES (?, ?, ?, ?, ?, ?)
                    """,
                    (
                        uid,
                        row["start_time"],
                        row["end_time"],
                        row.get("app_name"),
                        row.get("title"),
                        row.get("url"),
                    ),
                )

        for row in dataset.get("apps", []):
            app_name = _pick(row, "app_name", "aplikasi")
            jenis = _pick(row, "productivity_type", "jenis", default=0)
            productivity = row.get("productivity", 0)
            for_user = str(row.get("for_user", "0"))
            if "id" in row:
                prod_conn.execute(
                    """
                    INSERT INTO aplikasi (id, aplikasi, window_title, url, jenis, productivity, for_user)
                    VALUES (?, ?, ?, ?, ?, ?, ?)
                    """,
                    (
                        row["id"],
                        app_name,
                        row.get("window_title"),
                        row.get("url"),
                        jenis,
                        productivity,
                        for_user,
                    ),
                )
            else:
                prod_conn.execute(
                    """
                    INSERT INTO aplikasi (aplikasi, window_title, url, jenis, productivity, for_user)
                    VALUES (?, ?, ?, ?, ?, ?)
                    """,
                    (
                        app_name,
                        row.get("window_title"),
                        row.get("url"),
                        jenis,
                        productivity,
                        for_user,
                    ),
                )

        for row in dataset.get("tasks", []):
            task_desc = _pick(row, "task_desc", "task")
            if "id" in row:
                prod_conn.execute(
                    """
                    INSERT INTO task (id, project_name, task, max_time, time_usage, active, status, paused, user_id, created_at)
                    VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                    """,
                    (
                        row["id"],
                        row["project_name"],
                        task_desc,
                        row.get("max_time", 0),
                        row.get("time_usage", 0),
                        int(bool(row.get("active", 0))),
                        row.get("status", "pending"),
                        int(bool(row.get("paused", 0))),
                        row["user_id"],
                        row.get("created_at"),
                    ),
                )
            else:
                prod_conn.execute(
                    """
                    INSERT INTO task (project_name, task, max_time, time_usage, active, status, paused, user_id, created_at)
                    VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
                    """,
                    (
                        row["project_name"],
                        task_desc,
                        row.get("max_time", 0),
                        row.get("time_usage", 0),
                        int(bool(row.get("active", 0))),
                        row.get("status", "pending"),
                        int(bool(row.get("paused", 0))),
                        row["user_id"],
                        row.get("created_at"),
                    ),
                )

        for row in dataset.get("completed_tasks", []):
            task_desc = _pick(row, "task_desc", "task")
            if "id" in row:
                prod_conn.execute(
                    """
                    INSERT INTO completed_tasks (id, project_name, task, max_time, time_usage, completed_time, user_id)
                    VALUES (?, ?, ?, ?, ?, ?, ?)
                    """,
                    (
                        row["id"],
                        row.get("project_name"),
                        task_desc,
                        row.get("max_time"),
                        row.get("time_usage"),
                        row.get("completed_time"),
                        row.get("user_id"),
                    ),
                )
            else:
                prod_conn.execute(
                    """
                    INSERT INTO completed_tasks (project_name, task, max_time, time_usage, completed_time, user_id)
                    VALUES (?, ?, ?, ?, ?, ?)
                    """,
                    (
                        row.get("project_name"),
                        task_desc,
                        row.get("max_time"),
                        row.get("time_usage"),
                        row.get("completed_time"),
                        row.get("user_id"),
                    ),
                )

        if "idle_settings" in dataset and dataset["idle_settings"] is not None:
            for row in dataset["idle_settings"]:
                if "id" in row:
                    prod_conn.execute(
                        "INSERT INTO idle_settings (id, threshold_seconds) VALUES (?, ?)",
                        (row["id"], row.get("threshold_seconds", 180)),
                    )
                else:
                    prod_conn.execute(
                        "INSERT INTO idle_settings (threshold_seconds) VALUES (?)",
                        (row.get("threshold_seconds", 180),),
                    )
        else:
            cnt = prod_conn.execute("SELECT COUNT(*) FROM idle_settings").fetchone()[0]
            if cnt == 0:
                prod_conn.execute("INSERT INTO idle_settings (threshold_seconds) VALUES (180)")

        for row in dataset.get("pause_logs", []):
            paused_at = _pick(row, "paused_at", "start_reality")
            resumed_at = _pick(row, "resumed_at", "end_reality")
            status = _pick(row, "status", "current_status", default="paused")
            if "id" in row:
                prod_conn.execute(
                    """
                    INSERT INTO log_paused (id, task_id, start_reality, end_reality, current_status)
                    VALUES (?, ?, ?, ?, ?)
                    """,
                    (row["id"], row["task_id"], paused_at, resumed_at, status),
                )
            else:
                prod_conn.execute(
                    """
                    INSERT INTO log_paused (task_id, start_reality, end_reality, current_status)
                    VALUES (?, ?, ?, ?)
                    """,
                    (row["task_id"], paused_at, resumed_at, status),
                )

        for row in dataset.get("work_time", []):
            work_date = _pick(row, "work_date", "date")
            if "id" in row:
                prod_conn.execute(
                    """
                    INSERT INTO work_time (id, user_id, date, elapsed_seconds)
                    VALUES (?, ?, ?, ?)
                    """,
                    (row["id"], row["user_id"], work_date, row.get("elapsed_seconds", 0)),
                )
            else:
                prod_conn.execute(
                    """
                    INSERT INTO work_time (user_id, date, elapsed_seconds)
                    VALUES (?, ?, ?)
                    """,
                    (row["user_id"], work_date, row.get("elapsed_seconds", 0)),
                )

        act_conn.commit()
        prod_conn.commit()
    finally:
        act_conn.close()
        prod_conn.close()


def seed_v2_db(
    deskmon_db_path: str | Path,
    data: dict[str, Any] | None = None,
) -> None:
    """Create and seed DeskmonV2 unified SQLite database + 4 legacy views."""
    dataset = data or {}
    conn = _connect_rw(deskmon_db_path)
    try:
        conn.executescript(V2_UNIFIED_DDL)

        for u in dataset.get("users", []):
            if "id" in u:
                conn.execute(
                    """
                    INSERT INTO users (id, username, password, email, department, role, token, profile_image)
                    VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                    """,
                    (
                        u["id"],
                        u["username"],
                        u.get("password", ""),
                        u.get("email"),
                        u.get("department"),
                        u.get("role"),
                        u.get("token"),
                        u.get("profile_image"),
                    ),
                )
            else:
                conn.execute(
                    """
                    INSERT INTO users (username, password, email, department, role, token, profile_image)
                    VALUES (?, ?, ?, ?, ?, ?, ?)
                    """,
                    (
                        u["username"],
                        u.get("password", ""),
                        u.get("email"),
                        u.get("department"),
                        u.get("role"),
                        u.get("token"),
                        u.get("profile_image"),
                    ),
                )

        for row in dataset.get("logs", []):
            uid = _pick(row, "user_id", "id_user")
            if "id" in row:
                conn.execute(
                    """
                    INSERT INTO activity_logs (id, user_id, start_time, end_time, app_name, title, url)
                    VALUES (?, ?, ?, ?, ?, ?, ?)
                    """,
                    (
                        row["id"],
                        uid,
                        row["start_time"],
                        row["end_time"],
                        row.get("app_name"),
                        row.get("title"),
                        row.get("url"),
                    ),
                )
            else:
                conn.execute(
                    """
                    INSERT INTO activity_logs (user_id, start_time, end_time, app_name, title, url)
                    VALUES (?, ?, ?, ?, ?, ?)
                    """,
                    (
                        uid,
                        row["start_time"],
                        row["end_time"],
                        row.get("app_name"),
                        row.get("title"),
                        row.get("url"),
                    ),
                )

        for row in dataset.get("apps", []):
            app_name = _pick(row, "app_name", "aplikasi")
            ptype = _pick(row, "productivity_type", "jenis", default=0)
            productivity = row.get("productivity", 0)
            for_user = str(row.get("for_user", "0"))
            if "id" in row:
                conn.execute(
                    """
                    INSERT INTO productivity_apps (id, app_name, window_title, url, productivity_type, productivity, for_user)
                    VALUES (?, ?, ?, ?, ?, ?, ?)
                    """,
                    (
                        row["id"],
                        app_name,
                        row.get("window_title"),
                        row.get("url"),
                        ptype,
                        productivity,
                        for_user,
                    ),
                )
            else:
                conn.execute(
                    """
                    INSERT INTO productivity_apps (app_name, window_title, url, productivity_type, productivity, for_user)
                    VALUES (?, ?, ?, ?, ?, ?)
                    """,
                    (
                        app_name,
                        row.get("window_title"),
                        row.get("url"),
                        ptype,
                        productivity,
                        for_user,
                    ),
                )

        for row in dataset.get("tasks", []):
            task_desc = _pick(row, "task_desc", "task")
            if "id" in row:
                conn.execute(
                    """
                    INSERT INTO tasks (id, user_id, project_name, task_desc, max_time, time_usage, active, status, paused, created_at)
                    VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                    """,
                    (
                        row["id"],
                        row["user_id"],
                        row["project_name"],
                        task_desc,
                        row.get("max_time", 0),
                        row.get("time_usage", 0),
                        int(bool(row.get("active", 0))),
                        row.get("status", "pending"),
                        int(bool(row.get("paused", 0))),
                        row.get("created_at"),
                    ),
                )
            else:
                conn.execute(
                    """
                    INSERT INTO tasks (user_id, project_name, task_desc, max_time, time_usage, active, status, paused, created_at)
                    VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
                    """,
                    (
                        row["user_id"],
                        row["project_name"],
                        task_desc,
                        row.get("max_time", 0),
                        row.get("time_usage", 0),
                        int(bool(row.get("active", 0))),
                        row.get("status", "pending"),
                        int(bool(row.get("paused", 0))),
                        row.get("created_at"),
                    ),
                )

        for row in dataset.get("completed_tasks", []):
            task_desc = _pick(row, "task_desc", "task")
            if "id" in row:
                conn.execute(
                    """
                    INSERT INTO completed_tasks (id, user_id, project_name, task_desc, max_time, time_usage, completed_time)
                    VALUES (?, ?, ?, ?, ?, ?, ?)
                    """,
                    (
                        row["id"],
                        row["user_id"],
                        row.get("project_name"),
                        task_desc,
                        row.get("max_time"),
                        row.get("time_usage"),
                        row.get("completed_time"),
                    ),
                )
            else:
                conn.execute(
                    """
                    INSERT INTO completed_tasks (user_id, project_name, task_desc, max_time, time_usage, completed_time)
                    VALUES (?, ?, ?, ?, ?, ?)
                    """,
                    (
                        row["user_id"],
                        row.get("project_name"),
                        task_desc,
                        row.get("max_time"),
                        row.get("time_usage"),
                        row.get("completed_time"),
                    ),
                )

        if "idle_settings" in dataset and dataset["idle_settings"] is not None:
            for row in dataset["idle_settings"]:
                if "id" in row:
                    conn.execute(
                        "INSERT INTO idle_settings (id, threshold_seconds) VALUES (?, ?)",
                        (row["id"], row.get("threshold_seconds", 180)),
                    )
                else:
                    conn.execute(
                        "INSERT INTO idle_settings (threshold_seconds) VALUES (?)",
                        (row.get("threshold_seconds", 180),),
                    )
        else:
            cnt = conn.execute("SELECT COUNT(*) FROM idle_settings").fetchone()[0]
            if cnt == 0:
                conn.execute("INSERT INTO idle_settings (threshold_seconds) VALUES (180)")

        for row in dataset.get("pause_logs", []):
            paused_at = _pick(row, "paused_at", "start_reality")
            resumed_at = _pick(row, "resumed_at", "end_reality")
            status = _pick(row, "status", "current_status", default="paused")
            if "id" in row:
                conn.execute(
                    """
                    INSERT INTO task_pause_logs (id, task_id, paused_at, resumed_at, status)
                    VALUES (?, ?, ?, ?, ?)
                    """,
                    (row["id"], row["task_id"], paused_at, resumed_at, status),
                )
            else:
                conn.execute(
                    """
                    INSERT INTO task_pause_logs (task_id, paused_at, resumed_at, status)
                    VALUES (?, ?, ?, ?)
                    """,
                    (row["task_id"], paused_at, resumed_at, status),
                )

        for row in dataset.get("work_time", []):
            work_date = _pick(row, "work_date", "date")
            if "id" in row:
                conn.execute(
                    """
                    INSERT INTO work_time_records (id, user_id, work_date, elapsed_seconds)
                    VALUES (?, ?, ?, ?)
                    """,
                    (row["id"], row["user_id"], work_date, row.get("elapsed_seconds", 0)),
                )
            else:
                conn.execute(
                    """
                    INSERT INTO work_time_records (user_id, work_date, elapsed_seconds)
                    VALUES (?, ?, ?)
                    """,
                    (row["user_id"], work_date, row.get("elapsed_seconds", 0)),
                )

        conn.commit()
    finally:
        conn.close()


def read_1033_snapshot(
    activity_db_path: str | Path,
    productivity_db_path: str | Path,
) -> dict[str, list[dict[str, Any]]]:
    """Read Deskmon 1.0.3.3 dual DBs read-only into canonical sorted snapshot."""
    act_conn = _connect_ro(activity_db_path)
    prod_conn = _connect_ro(productivity_db_path)
    try:
        users = [
            {
                "id": r["id"],
                "username": r["username"],
                "password": r["password"],
                "email": r["email"],
                "department": r["department"],
                "role": r["role"],
                "token": r["token"],
                "profile_image": r["profile_image"],
            }
            for r in act_conn.execute("SELECT * FROM users ORDER BY id").fetchall()
        ]
        logs = [
            {
                "id": r["id"],
                "user_id": r["id_user"],
                "start_time": r["start_time"],
                "end_time": r["end_time"],
                "app_name": r["app_name"],
                "title": r["title"],
                "url": r["url"],
            }
            for r in act_conn.execute("SELECT * FROM log ORDER BY id").fetchall()
        ]
        apps = [
            {
                "id": r["id"],
                "app_name": r["aplikasi"],
                "window_title": r["window_title"],
                "url": r["url"],
                "productivity_type": r["jenis"],
                "productivity": r["productivity"],
                "for_user": r["for_user"],
            }
            for r in prod_conn.execute("SELECT * FROM aplikasi ORDER BY id").fetchall()
        ]
        tasks = [
            {
                "id": r["id"],
                "user_id": r["user_id"],
                "project_name": r["project_name"],
                "task_desc": r["task"],
                "max_time": r["max_time"],
                "time_usage": r["time_usage"],
                "active": int(bool(r["active"])),
                "status": r["status"],
                "paused": int(bool(r["paused"])),
                "created_at": r["created_at"],
            }
            for r in prod_conn.execute("SELECT * FROM task ORDER BY id").fetchall()
        ]
        completed_tasks = [
            {
                "id": r["id"],
                "user_id": r["user_id"],
                "project_name": r["project_name"],
                "task_desc": r["task"],
                "max_time": r["max_time"],
                "time_usage": r["time_usage"],
                "completed_time": r["completed_time"],
            }
            for r in prod_conn.execute("SELECT * FROM completed_tasks ORDER BY id").fetchall()
        ]
        idle_settings = [
            {
                "id": r["id"],
                "threshold_seconds": r["threshold_seconds"],
            }
            for r in prod_conn.execute("SELECT * FROM idle_settings ORDER BY id").fetchall()
        ]
        pause_logs = [
            {
                "id": r["id"],
                "task_id": r["task_id"],
                "paused_at": r["start_reality"],
                "resumed_at": r["end_reality"],
                "status": r["current_status"],
            }
            for r in prod_conn.execute("SELECT * FROM log_paused ORDER BY id").fetchall()
        ]
        work_time = [
            {
                "id": r["id"],
                "user_id": r["user_id"],
                "work_date": r["date"],
                "elapsed_seconds": r["elapsed_seconds"],
            }
            for r in prod_conn.execute("SELECT * FROM work_time ORDER BY id").fetchall()
        ]
        return {
            "users": users,
            "logs": logs,
            "apps": apps,
            "tasks": tasks,
            "completed_tasks": completed_tasks,
            "idle_settings": idle_settings,
            "pause_logs": pause_logs,
            "work_time": work_time,
        }
    finally:
        act_conn.close()
        prod_conn.close()


def read_v2_snapshot(deskmon_db_path: str | Path) -> dict[str, list[dict[str, Any]]]:
    """Read DeskmonV2 unified DB read-only into canonical sorted snapshot."""
    conn = _connect_ro(deskmon_db_path)
    try:
        users = [
            {
                "id": r["id"],
                "username": r["username"],
                "password": r["password"],
                "email": r["email"],
                "department": r["department"],
                "role": r["role"],
                "token": r["token"],
                "profile_image": r["profile_image"],
            }
            for r in conn.execute("SELECT * FROM users ORDER BY id").fetchall()
        ]
        logs = [
            {
                "id": r["id"],
                "user_id": r["user_id"],
                "start_time": r["start_time"],
                "end_time": r["end_time"],
                "app_name": r["app_name"],
                "title": r["title"],
                "url": r["url"],
            }
            for r in conn.execute("SELECT * FROM activity_logs ORDER BY id").fetchall()
        ]
        apps = [
            {
                "id": r["id"],
                "app_name": r["app_name"],
                "window_title": r["window_title"],
                "url": r["url"],
                "productivity_type": r["productivity_type"],
                "productivity": r["productivity"],
                "for_user": r["for_user"],
            }
            for r in conn.execute("SELECT * FROM productivity_apps ORDER BY id").fetchall()
        ]
        tasks = [
            {
                "id": r["id"],
                "user_id": r["user_id"],
                "project_name": r["project_name"],
                "task_desc": r["task_desc"],
                "max_time": r["max_time"],
                "time_usage": r["time_usage"],
                "active": int(bool(r["active"])),
                "status": r["status"],
                "paused": int(bool(r["paused"])),
                "created_at": r["created_at"],
            }
            for r in conn.execute("SELECT * FROM tasks ORDER BY id").fetchall()
        ]
        completed_tasks = [
            {
                "id": r["id"],
                "user_id": r["user_id"],
                "project_name": r["project_name"],
                "task_desc": r["task_desc"],
                "max_time": r["max_time"],
                "time_usage": r["time_usage"],
                "completed_time": r["completed_time"],
            }
            for r in conn.execute("SELECT * FROM completed_tasks ORDER BY id").fetchall()
        ]
        idle_settings = [
            {
                "id": r["id"],
                "threshold_seconds": r["threshold_seconds"],
            }
            for r in conn.execute("SELECT * FROM idle_settings ORDER BY id").fetchall()
        ]
        pause_logs = [
            {
                "id": r["id"],
                "task_id": r["task_id"],
                "paused_at": r["paused_at"],
                "resumed_at": r["resumed_at"],
                "status": r["status"],
            }
            for r in conn.execute("SELECT * FROM task_pause_logs ORDER BY id").fetchall()
        ]
        work_time = [
            {
                "id": r["id"],
                "user_id": r["user_id"],
                "work_date": r["work_date"],
                "elapsed_seconds": r["elapsed_seconds"],
            }
            for r in conn.execute("SELECT * FROM work_time_records ORDER BY id").fetchall()
        ]
        return {
            "users": users,
            "logs": logs,
            "apps": apps,
            "tasks": tasks,
            "completed_tasks": completed_tasks,
            "idle_settings": idle_settings,
            "pause_logs": pause_logs,
            "work_time": work_time,
        }
    finally:
        conn.close()


def read_v2_legacy_views_snapshot(
    deskmon_db_path: str | Path,
) -> dict[str, list[dict[str, Any]]]:
    """Query the 4 DeskmonV2 legacy compatibility views read-only."""
    conn = _connect_ro(deskmon_db_path)
    try:
        return {
            "log": [
                dict(r) for r in conn.execute("SELECT * FROM log ORDER BY id").fetchall()
            ],
            "aplikasi": [
                dict(r)
                for r in conn.execute("SELECT * FROM aplikasi ORDER BY id").fetchall()
            ],
            "task": [
                dict(r) for r in conn.execute("SELECT * FROM task ORDER BY id").fetchall()
            ],
            "work_time": [
                dict(r)
                for r in conn.execute("SELECT * FROM work_time ORDER BY id").fetchall()
            ],
        }
    finally:
        conn.close()


def schema_check_v2(deskmon_db_path: str | Path) -> dict[str, Any]:
    """Inspect DeskmonV2 SQLite schema (tables, views, columns) read-only."""
    conn = _connect_ro(deskmon_db_path)
    try:
        rows = conn.execute(
            "SELECT name, type FROM sqlite_master WHERE type IN ('table', 'view') AND name NOT LIKE 'sqlite_%'"
        ).fetchall()
        tables = {r["name"] for r in rows if r["type"] == "table"}
        views = {r["name"] for r in rows if r["type"] == "view"}
        columns: dict[str, list[str]] = {}
        for name in sorted(tables | views):
            cols = conn.execute(f'PRAGMA table_info("{name}")').fetchall()
            columns[name] = [c["name"] for c in cols]
        return {"tables": tables, "views": views, "columns": columns}
    finally:
        conn.close()


def diff_snapshots(
    expected: dict[str, list[dict[str, Any]]],
    actual: dict[str, list[dict[str, Any]]],
    tables: list[str] | None = None,
    ignore_fields: set[str] | None = None,
) -> list[str]:
    """Compare two canonical snapshots and return human-readable mismatches."""
    diffs: list[str] = []
    ignored = ignore_fields or set()
    table_names = tables if tables is not None else sorted(set(expected.keys()) | set(actual.keys()))

    for tbl in table_names:
        exp_rows = expected.get(tbl, [])
        act_rows = actual.get(tbl, [])
        if len(exp_rows) != len(act_rows):
            diffs.append(
                f"[{tbl}] row count mismatch: expected {len(exp_rows)}, got {len(act_rows)}"
            )
        for idx in range(min(len(exp_rows), len(act_rows))):
            exp_row = {k: v for k, v in exp_row_items(exp_rows[idx]) if k not in ignored}
            act_row = {k: v for k, v in exp_row_items(act_rows[idx]) if k not in ignored}
            if exp_row != act_row:
                diffs.append(
                    f"[{tbl}] row[{idx}] mismatch: expected {exp_row!r}, got {act_row!r}"
                )
    return diffs


def exp_row_items(row: dict[str, Any]) -> list[tuple[str, Any]]:
    return sorted(row.items(), key=lambda kv: kv[0])


def assert_db_parity(
    expected: dict[str, list[dict[str, Any]]],
    actual: dict[str, list[dict[str, Any]]],
    tables: list[str] | None = None,
    ignore_fields: set[str] | None = None,
) -> None:
    """Assert exact parity between two DB snapshots, raising AssertionError on diff."""
    diffs = diff_snapshots(expected, actual, tables=tables, ignore_fields=ignore_fields)
    if diffs:
        raise AssertionError("Database parity mismatch:\n  " + "\n  ".join(diffs))
