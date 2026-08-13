#!/usr/bin/env python3
"""Validate repository-owned skills and offline GitHub Actions guardrails."""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SKILLS_ROOT = ROOT / ".agents" / "skills"
WORKFLOWS_ROOT = ROOT / ".github" / "workflows"
NAME_RE = re.compile(r"^[a-z0-9]+(?:-[a-z0-9]+)*$")
QUOTED_FIELD_RE = re.compile(r'^\s{2}(display_name|short_description|default_prompt): "(.+)"$')
ACTION_USE_RE = re.compile(r"(?m)^\s+-?\s*uses:\s+([^@\s]+)@([^\s#]+)")
FULL_SHA_RE = re.compile(r"^[0-9a-f]{40}$")


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


def validate_workflows(errors: list[str]) -> int:
    required = {
        "validation.yml",
        "deep-validation.yml",
        "offline-artifact-delivery.yml",
    }
    present = {path.name for path in WORKFLOWS_ROOT.glob("*.y*ml")}
    missing = required - present
    if missing:
        fail(errors, f"missing required workflows: {', '.join(sorted(missing))}")

    forbidden = {
        "pull_request_target": "privileged pull-request trigger",
        "permissions: write": "write-all permissions",
        "contents: write": "repository-content write permission",
        "secrets.": "repository/environment secret reference",
        "self-hosted": "self-hosted runner",
        "TRADEBOT_ENABLE_CTRADER_GATE6=ON": "Gate 6 enablement",
        "TRADEBOT_ENABLE_CTRADER_GATE7=ON": "Gate 7 enablement",
        "gh release": "GitHub Release publication",
        "actions/create-release": "GitHub Release publication",
    }

    count = 0
    for workflow in sorted(WORKFLOWS_ROOT.glob("*.y*ml")):
        count += 1
        text = workflow.read_text(encoding="utf-8")
        relative = workflow.relative_to(ROOT)
        if not re.search(r"(?m)^permissions:\n\s+contents: read\s*$", text):
            fail(errors, f"{relative}: missing top-level read-only contents permission")
        checkout_count = text.count("uses: actions/checkout@")
        if text.count("persist-credentials: false") != checkout_count:
            fail(errors, f"{relative}: every checkout must disable persisted credentials")
        for action, action_ref in ACTION_USE_RE.findall(text):
            if not FULL_SHA_RE.fullmatch(action_ref):
                fail(errors, f"{relative}: {action} must be pinned to a full commit SHA")
        for token, reason in forbidden.items():
            if token in text:
                fail(errors, f"{relative}: forbidden {reason}: {token}")

    delivery = WORKFLOWS_ROOT / "offline-artifact-delivery.yml"
    if delivery.is_file():
        text = delivery.read_text(encoding="utf-8")
        trigger_section = text.split("permissions:", 1)[0]
        if "workflow_dispatch:" not in trigger_section:
            fail(errors, f"{delivery.relative_to(ROOT)}: delivery must be manually dispatched")
        for trigger in ("push:", "pull_request:", "schedule:", "release:"):
            if trigger in trigger_section:
                fail(errors, f"{delivery.relative_to(ROOT)}: delivery cannot use {trigger.rstrip(':')} trigger")
        for required_token in (
            "test ! -e tradebot_core",
            "artifact_class=offline_validation_evidence",
            "packaged_executable=false",
            "release_authorized=false",
            "deployment_authorized=false",
            "live_trading_authorized=false",
        ):
            if required_token not in text:
                fail(errors, f"{delivery.relative_to(ROOT)}: missing evidence guard: {required_token}")
    return count


def main() -> int:
    errors: list[str] = []
    skill_count = validate_skills(errors)
    workflow_count = validate_workflows(errors)
    if errors:
        for error in errors:
            print(f"ERROR: {error}", file=sys.stderr)
        return 1
    print(f"Validated {skill_count} skills and {workflow_count} offline workflows.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
