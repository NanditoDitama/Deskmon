"""CLI test runner for Deskmon E2E & 1.0.3.3 parity test suite."""

from __future__ import annotations

import argparse
import datetime as _dt
import json
import os
import sys
import time
import unittest
from pathlib import Path
from typing import Any

E2E_ROOT = Path(__file__).resolve().parent
REPO_ROOT = E2E_ROOT.parent.parent

if str(E2E_ROOT) not in sys.path:
    sys.path.insert(0, str(E2E_ROOT))

from harness.process_env import ensure_e2e_build, find_ref_binary  # noqa: E402

TIER_ORDER = ("smoke", "tier1", "tier2", "tier3", "tier4")


def _tier_from_test_id(test_id: str) -> str:
    lower = test_id.lower()
    if "test_harness_smoke" in lower or lower.startswith("harness.") or ".harness." in lower:
        return "smoke"
    for t in ("tier1", "tier2", "tier3", "tier4"):
        if lower.startswith(f"{t}.") or f".{t}." in lower or f"test_t{t[-1]}_" in lower:
            return t
    return "other"


class TieredTestResult(unittest.TextTestResult):
    """Collects per-tier counts, durations, and structured failure details."""

    def __init__(self, stream: Any, descriptions: bool, verbosity: int) -> None:
        super().__init__(stream, descriptions, verbosity)
        self.tier_stats: dict[str, dict[str, Any]] = {}
        self._test_start_times: dict[str, float] = {}
        self.structured_failures: list[dict[str, str]] = []
        self.structured_errors: list[dict[str, str]] = []
        self.structured_skipped: list[dict[str, str]] = []

    def _ensure_tier(self, tier: str) -> dict[str, Any]:
        if tier not in self.tier_stats:
            self.tier_stats[tier] = {
                "run": 0,
                "passed": 0,
                "failed": 0,
                "errors": 0,
                "skipped": 0,
                "unexpected_successes": 0,
                "duration_s": 0.0,
            }
        return self.tier_stats[tier]

    def startTest(self, test: unittest.TestCase) -> None:
        super().startTest(test)
        self._test_start_times[test.id()] = time.monotonic()

    def stopTest(self, test: unittest.TestCase) -> None:
        super().stopTest(test)
        elapsed = time.monotonic() - self._test_start_times.pop(test.id(), time.monotonic())
        tier = _tier_from_test_id(test.id())
        stats = self._ensure_tier(tier)
        stats["run"] += 1
        stats["duration_s"] += elapsed

    def addSuccess(self, test: unittest.TestCase) -> None:
        super().addSuccess(test)
        tier = _tier_from_test_id(test.id())
        self._ensure_tier(tier)["passed"] += 1

    def addFailure(self, test: unittest.TestCase, err: Any) -> None:
        super().addFailure(test, err)
        tier = _tier_from_test_id(test.id())
        self._ensure_tier(tier)["failed"] += 1
        self.structured_failures.append(
            {
                "test": test.id(),
                "tier": tier,
                "traceback": self._exc_info_to_string(err, test),
            }
        )

    def addError(self, test: unittest.TestCase, err: Any) -> None:
        super().addError(test, err)
        tier = _tier_from_test_id(test.id())
        self._ensure_tier(tier)["errors"] += 1
        self.structured_errors.append(
            {
                "test": test.id(),
                "tier": tier,
                "traceback": self._exc_info_to_string(err, test),
            }
        )

    def addSkip(self, test: unittest.TestCase, reason: str) -> None:
        super().addSkip(test, reason)
        tier = _tier_from_test_id(test.id())
        self._ensure_tier(tier)["skipped"] += 1
        self.structured_skipped.append(
            {
                "test": test.id(),
                "tier": tier,
                "reason": reason,
            }
        )

    def addExpectedFailure(self, test: unittest.TestCase, err: Any) -> None:
        super().addExpectedFailure(test, err)
        tier = _tier_from_test_id(test.id())
        self._ensure_tier(tier)["passed"] += 1

    def addUnexpectedSuccess(self, test: unittest.TestCase) -> None:
        super().addUnexpectedSuccess(test)
        tier = _tier_from_test_id(test.id())
        self._ensure_tier(tier)["unexpected_successes"] += 1


class TieredTestRunner(unittest.TextTestRunner):
    resultclass = TieredTestResult  # type: ignore[assignment]


def _discover_suite(
    tier: str,
    area: str | None,
    name_pattern: str | None,
) -> unittest.TestSuite:
    loader = unittest.TestLoader()
    if name_pattern:
        loader.testNamePatterns = [f"*{name_pattern}*"]

    suite = unittest.TestSuite()
    area_lower = area.lower() if area else None

    if tier == "smoke":
        targets = [("smoke", E2E_ROOT / "harness", "test_harness_smoke.py")]
    elif tier in ("1", "2", "3", "4"):
        t_dir = E2E_ROOT / f"tier{tier}"
        if tier in ("1", "2") and area_lower:
            pat = f"test_t{tier}_{area_lower}_*.py"
        else:
            pat = "test_*.py"
        targets = [(f"tier{tier}", t_dir, pat)]
    elif tier == "all":
        targets = [("smoke", E2E_ROOT / "harness", "test_harness_smoke.py")]
        for t_num in ("1", "2", "3", "4"):
            t_dir = E2E_ROOT / f"tier{t_num}"
            if not t_dir.is_dir():
                continue
            if t_num in ("1", "2") and area_lower:
                pat = f"test_t{t_num}_{area_lower}_*.py"
            else:
                pat = "test_*.py"
            targets.append((f"tier{t_num}", t_dir, pat))
    else:
        raise ValueError(f"Unsupported tier: {tier}")

    for _, start_dir, file_pattern in targets:
        if not start_dir.is_dir():
            continue
        discovered = loader.discover(
            start_dir=str(start_dir),
            pattern=file_pattern,
            top_level_dir=str(E2E_ROOT),
        )
        suite.addTests(discovered)

    return suite


def _print_summary(
    tiers: dict[str, dict[str, Any]],
    totals: dict[str, Any],
) -> None:
    print("\n=== Deskmon E2E Summary ===")
    print("Tier     | Run | Pass | Fail | Err | Skip | Time(s)")
    ordered_keys = [k for k in TIER_ORDER if k in tiers] + [
        k for k in sorted(tiers.keys()) if k not in TIER_ORDER
    ]
    for tier_name in ordered_keys:
        s = tiers[tier_name]
        print(
            f"{tier_name:<8} | {s['run']:>3} | {s['passed']:>4} | "
            f"{s['failed']:>4} | {s['errors']:>3} | {s['skipped']:>4} | "
            f"{s['duration_s']:>7.2f}"
        )
    print(
        f"{'TOTAL':<8} | {totals['run']:>3} | {totals['passed']:>4} | "
        f"{totals['failed']:>4} | {totals['errors']:>3} | {totals['skipped']:>4} | "
        f"{totals['duration_s']:>7.2f}"
    )


def _write_json_report(
    report_path: Path,
    *,
    v2_bin: str | None,
    ref_bin: str | None,
    tiers: dict[str, dict[str, Any]],
    totals: dict[str, Any],
    failures: list[dict[str, str]],
    errors: list[dict[str, str]],
    skipped: list[dict[str, str]],
    exit_code: int,
) -> None:
    report_path.parent.mkdir(parents=True, exist_ok=True)
    payload = {
        "timestamp_utc": _dt.datetime.now(_dt.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "v2_bin": v2_bin,
        "ref_bin": ref_bin,
        "tiers": tiers,
        "totals": totals,
        "failures": failures,
        "errors": errors,
        "skipped": skipped,
        "exit_code": exit_code,
    }
    report_path.write_text(json.dumps(payload, indent=2), encoding="utf-8")


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Deskmon E2E & 1.0.3.3 Parity Test Runner")
    parser.add_argument(
        "--tier",
        choices=["1", "2", "3", "4", "all", "smoke"],
        default="all",
        help="Test tier to execute (default: all)",
    )
    parser.add_argument(
        "--area",
        choices=["A", "B", "C", "D", "E", "F", "G", "H"],
        default=None,
        help="Filter Tier 1/2 tests by feature area A-H",
    )
    parser.add_argument(
        "--v2-bin",
        type=Path,
        default=None,
        help="Explicit path to DeskmonV2 Deskmon.exe (skips build)",
    )
    parser.add_argument(
        "--ref-bin",
        type=Path,
        default=None,
        help="Explicit path to Deskmon 1.0.3.3 Release Deskmon.exe",
    )
    parser.add_argument(
        "--build-dir",
        type=Path,
        default=REPO_ROOT / "build_e2e",
        help="CMake build directory for DeskmonV2 E2E binary (default: build_e2e)",
    )
    parser.add_argument(
        "--skip-build",
        action="store_true",
        help="Use existing <build-dir>/Deskmon.exe without running cmake/ninja",
    )
    parser.add_argument(
        "--headless",
        action="store_true",
        help="Force headless offscreen mode (DESKMON_E2E_HEADLESS=1, QT_QPA_PLATFORM=offscreen)",
    )
    parser.add_argument(
        "--json-report",
        type=Path,
        default=None,
        help="Write structured JSON summary report to this path",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="Verbose unittest output (verbosity=2)",
    )
    parser.add_argument(
        "-k",
        "--pattern",
        type=str,
        default=None,
        help="Substring filter for test method/class names",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)

    if args.headless:
        os.environ["DESKMON_E2E_HEADLESS"] = "1"
        os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

    suite = _discover_suite(tier=args.tier, area=args.area, name_pattern=args.pattern)
    if suite.countTestCases() == 0:
        msg = f"No tests discovered for tier={args.tier!r}"
        if args.area:
            msg += f", area={args.area!r}"
        if args.pattern:
            msg += f", pattern={args.pattern!r}"
        print(msg, file=sys.stderr)
        empty_totals = {
            "run": 0,
            "passed": 0,
            "failed": 0,
            "errors": 0,
            "skipped": 0,
            "unexpected_successes": 0,
            "duration_s": 0.0,
        }
        _print_summary({}, empty_totals)
        if args.json_report:
            _write_json_report(
                args.json_report,
                v2_bin=None,
                ref_bin=None,
                tiers={},
                totals=empty_totals,
                failures=[],
                errors=[{"test": "<discovery>", "tier": args.tier, "traceback": msg}],
                skipped=[],
                exit_code=2,
            )
        return 2

    v2_bin_path: Path | None = None
    try:
        if args.v2_bin is not None:
            v2_bin_path = args.v2_bin.resolve()
            if not v2_bin_path.is_file():
                raise FileNotFoundError(f"Explicit --v2-bin not found: {v2_bin_path}")
        elif args.skip_build:
            v2_bin_path = args.build_dir.resolve() / "Deskmon.exe"
            if not v2_bin_path.is_file():
                raise FileNotFoundError(
                    f"Existing Deskmon.exe not found at {v2_bin_path} with --skip-build"
                )
        else:
            v2_bin_path = ensure_e2e_build(REPO_ROOT, build_dir=args.build_dir)
    except Exception as exc:
        print(f"Build/binary resolution error: {exc}", file=sys.stderr)
        empty_totals = {
            "run": 0,
            "passed": 0,
            "failed": 0,
            "errors": 1,
            "skipped": 0,
            "unexpected_successes": 0,
            "duration_s": 0.0,
        }
        if args.json_report:
            _write_json_report(
                args.json_report,
                v2_bin=str(v2_bin_path) if v2_bin_path else None,
                ref_bin=None,
                tiers={},
                totals=empty_totals,
                failures=[],
                errors=[{"test": "<build>", "tier": args.tier, "traceback": str(exc)}],
                skipped=[],
                exit_code=2,
            )
        return 2

    os.environ["DESKMON_E2E_V2_BIN"] = str(v2_bin_path)

    ref_bin_path = find_ref_binary(args.ref_bin)
    if ref_bin_path is not None:
        os.environ["DESKMON_E2E_REF_BIN"] = str(ref_bin_path)

    verbosity = 2 if args.verbose else 1
    runner = TieredTestRunner(stream=sys.stdout, verbosity=verbosity)
    result: TieredTestResult = runner.run(suite)  # type: ignore[assignment]

    totals = {
        "run": sum(s["run"] for s in result.tier_stats.values()),
        "passed": sum(s["passed"] for s in result.tier_stats.values()),
        "failed": sum(s["failed"] for s in result.tier_stats.values()),
        "errors": sum(s["errors"] for s in result.tier_stats.values()),
        "skipped": sum(s["skipped"] for s in result.tier_stats.values()),
        "unexpected_successes": sum(
            s["unexpected_successes"] for s in result.tier_stats.values()
        ),
        "duration_s": sum(s["duration_s"] for s in result.tier_stats.values()),
    }

    _print_summary(result.tier_stats, totals)

    if totals["failed"] > 0 or totals["errors"] > 0 or totals["unexpected_successes"] > 0:
        exit_code = 1
    elif totals["run"] == 0:
        exit_code = 2
    else:
        exit_code = 0

    if args.json_report:
        _write_json_report(
            args.json_report,
            v2_bin=str(v2_bin_path),
            ref_bin=str(ref_bin_path) if ref_bin_path else None,
            tiers=result.tier_stats,
            totals=totals,
            failures=result.structured_failures,
            errors=result.structured_errors,
            skipped=result.structured_skipped,
            exit_code=exit_code,
        )

    return exit_code


if __name__ == "__main__":
    raise SystemExit(main())
