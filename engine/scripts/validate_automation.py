#!/usr/bin/env python3
"""Validate repository-owned skills and local offline guardrails."""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SKILLS_ROOT = ROOT / ".agents" / "skills"
NAME_RE = re.compile(r"^[a-z0-9]+(?:-[a-z0-9]+)*$")
QUOTED_FIELD_RE = re.compile(r'^\s{2}(display_name|short_description|default_prompt): "(.+)"$')
LOCAL_GUARDRAILS = (
    "ci_validate.sh",
    "ci_deep_validate.sh",
    "ci_policy_checks.sh",
    "package_offline_artifact.sh",
)


def fail(errors: list[str], message: str) -> None:
    errors.append(message)


def parse_frontmatter(path: Path, errors: list[str]) -> dict[str, str]:
    text = path.read_text(encoding="utf-8")
    if not text.startswith("---\n") or "\n---\n" not in text[4:]:
        fail(errors, f"{path.relative_to(ROOT)}: missing YAML frontmatter")
        return {}

    frontmatter, _ = text[4:].split("\n---\n", 1)
    fields: dict[str, str] = {}
    for line in frontmatter.splitlines():
        key, separator, value = line.partition(":")
        if not separator or not key or not value.strip():
            fail(errors, f"{path.relative_to(ROOT)}: invalid frontmatter line: {line!r}")
            continue
        fields[key.strip()] = value.strip()
    return fields


def validate_skills(errors: list[str]) -> int:
    count = 0
    for skill_dir in sorted(path for path in SKILLS_ROOT.iterdir() if path.is_dir()):
        count += 1
        skill_file = skill_dir / "SKILL.md"
        if not skill_file.is_file():
            fail(errors, f"{skill_dir.relative_to(ROOT)}: missing SKILL.md")
            continue

        fields = parse_frontmatter(skill_file, errors)
        if set(fields) != {"name", "description"}:
            fail(errors, f"{skill_file.relative_to(ROOT)}: frontmatter must contain only name and description")
        name = fields.get("name", "")
        if name != skill_dir.name:
            fail(errors, f"{skill_file.relative_to(ROOT)}: name does not match directory")
        if not NAME_RE.fullmatch(name) or len(name) > 64:
            fail(errors, f"{skill_file.relative_to(ROOT)}: invalid skill name")
        description = fields.get("description", "")
        if len(description) < 40 or "TODO" in description:
            fail(errors, f"{skill_file.relative_to(ROOT)}: description is incomplete")

        metadata_file = skill_dir / "agents" / "openai.yaml"
        if metadata_file.is_file():
            metadata: dict[str, str] = {}
            for line in metadata_file.read_text(encoding="utf-8").splitlines():
                match = QUOTED_FIELD_RE.fullmatch(line)
                if match:
                    metadata[match.group(1)] = match.group(2)
            if set(metadata) != {"display_name", "short_description", "default_prompt"}:
                fail(errors, f"{metadata_file.relative_to(ROOT)}: incomplete interface metadata")
            short_description = metadata.get("short_description", "")
            if not 25 <= len(short_description) <= 64:
                fail(errors, f"{metadata_file.relative_to(ROOT)}: short_description must be 25-64 characters")
            if f"${name}" not in metadata.get("default_prompt", ""):
                fail(errors, f"{metadata_file.relative_to(ROOT)}: default_prompt must name ${name}")
    return count


def validate_local_guardrails(errors: list[str]) -> int:
    if (ROOT / ".github").exists():
        fail(errors, ".github must not exist in the current Mynyra Engine tree")

    for script_name in LOCAL_GUARDRAILS:
        script_path = ROOT / "scripts" / script_name
        if not script_path.is_file():
            fail(errors, f"scripts/{script_name}: missing local guardrail")
            continue
        if not script_path.stat().st_mode & 0o111:
            fail(errors, f"scripts/{script_name}: local guardrail is not executable")

    policy_path = ROOT / "scripts" / "ci_policy_checks.sh"
    if policy_path.is_file():
        policy_text = policy_path.read_text(encoding="utf-8")
        for required_token in (
            "TRADEBOT_ENABLE_LIVE_RUNTIME",
            "TRADEBOT_ENABLE_CTRADER_GATE6",
            "TRADEBOT_ENABLE_CTRADER_GATE7",
            "TRADEBOT_ENABLE_CTRADER_DEMO",
            "SystemMode::BACKTEST",
            "retired GitHub automation",
        ):
            if required_token not in policy_text:
                fail(errors, f"scripts/ci_policy_checks.sh: missing local safety guard: {required_token}")
    return len(LOCAL_GUARDRAILS)


def main() -> int:
    errors: list[str] = []
    skill_count = validate_skills(errors)
    guardrail_count = validate_local_guardrails(errors)
    if errors:
        for error in errors:
            print(f"ERROR: {error}", file=sys.stderr)
        return 1
    print(f"Validated {skill_count} skills and {guardrail_count} local offline guardrails.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
