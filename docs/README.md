# Mynyra Documentation Index


## Purpose And Authority


- Purpose: index the active Mynyra documentation system and explain how its documents relate.
- Authority level: general documentation index; it does not override `AGENTS.md`, ADRs, risk policy, architecture, or active plans.
- Audience: operator, maintainers, Codex, contributors, reviewers, testers, and research agents.


This directory contains the active project documentation for Mynyra. Mynyra is the product-level home for the broader system around TradeBot; its current product boundary is offline and non-trading, and planned or unavailable capabilities must not be presented as live behavior. Historical offline/intranet-era material is not part of the active workflow unless reintroduced by an accepted ADR. ADR 0001 deprecates the old MOP/MOR/SS workflow.


## Authority Order


1. Explicit operator instruction for the current task.
2. Root `../AGENTS.md`.
3. Accepted ADRs in `decisions/`.
4. `RISK_POLICY.md`.
5. `ARCHITECTURE.md`.
6. Active approved plan using `../PLANS.md`.
7. `PROJECT_STATE.md`.
8. `ROADMAP.md`.
9. `WORKFLOW.md`.
10. `CODEX_EXECUTION_EVIDENCE.md`.
11. Skill-specific instructions in `../.agents/skills/`.
12. `../CONTRIBUTING.md`.
13. General documentation.


Financial safety and operator authority cannot be silently overridden by any document.


## Active Documents


- `PROJECT_STATE.md`: current verified repository-state summary, constraints, and next safe action; it points to the workstream map and roadmap phase authority.
- `REPOSITORY_REMEDIATION_PROGRAM.md`: canonical current focus lock,
  infrastructure-foundation definition, and WP-0 through WP-8 scope,
  dependencies, acceptance, tests, approvals, and rollback.
- `WORKSTREAM_ARCHITECTURE.md`: conceptually accepted TradeBot Workstream Architecture v1.0 project-level map, Workstream II amendment, Phase 23 evidence rhythm, ownership, and non-authorization gates.
- `ROADMAP.md`: deterministic phase authority within the Workstreams I-VII project map, including sequence, statuses, gates, and validation boundaries.
- `ARCHITECTURE.md`: system purpose, component boundaries, data/control flow, and architectural debt.
- `RISK_POLICY.md`: financial, order-execution, data, credential, reproducibility, and AI-agent risk controls.
- `TESTING.md`: test layers, commands, fixtures, deterministic checks, and minimum evidence.
- `DATA_POLICY.md`: historical/sample/generated data governance, provenance, checksums, schemas, and retention.
- `SECURITY.md`: credential storage, `.env`, redaction, dependency, shell, network, and incident-response policy.
- `ACTORS.md`: multi-actor roles, permissions, evidence, escalation, onboarding, and offboarding.
- `WORKFLOW.md`: end-to-end task intake, planning, implementation, verification, review, handoff, and professional halt.
- `CODEX_EXECUTION_EVIDENCE.md`: durable action-authorization, evidence-epoch,
  exact-artifact, retry-budget, shared-resource, sensitive-memory, and external-
  process rules for repository-side agents.
- `HANDOFF.md`: copy-pasteable session and actor handoff template.
- `BENCHMARKING.md`: performance evidence, benchmark commands, generated outputs, and claim rules.
- `DEPENDENCY_POLICY.md`: dependency review and offline-first dependency handling.
- `CONFIGURATION.md`: verified modes, CLI flags, env vars, paths, and configuration boundaries.
- `STYLE_GUIDE.md`: C++ and Markdown style guidance for current unconfigured tooling.
- `FAILURE_RECOVERY.md`: recovery from interrupted work, failed checks, stale state, and containment events.
- `LIVE_TRADING_READINESS.md`: explicit live-trading unlock checklist.
- `GLOSSARY.md`: shared terms for modes, actors, phases, replay, risk, and benchmarks.
- `REVIEW_CHECKLIST.md`: practical review checklist for source, docs, risk, tests, data, and security.
- `RELEASE_POLICY.md`: commit, push, release, and live-transition gates.
- `WORKSTREAM_I_INTEGRATION_ARCHITECTURE.md`: approved Phase 21 broker-neutral integration architecture and subsystem boundaries.
- `WORKSTREAM_I_ADAPTER_CONTRACT.md`: approved Phase 21 adapter lifecycle, event, and ownership contract.
