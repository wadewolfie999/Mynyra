/**
 * Design: Night Operations Manual — an offline field desk with evidence rails,
 * visible system boundaries, editorial hierarchy, and no implied live capability.
 */
import { useState } from "react";
import { toast } from "sonner";
import {
  Archive,
  ArrowUpRight,
  BookOpenCheck,
  Box,
  ChevronRight,
  CircleAlert,
  ClipboardCheck,
  Compass,
  FileText,
  GitBranch,
  LockKeyhole,
  Network,
  ShieldCheck,
  Sparkles,
} from "lucide-react";

type PanelKey = "overview" | "system-map" | "evidence";

const panels: Record<
  PanelKey,
  { label: string; eyebrow: string; title: string; description: string }
> = {
  overview: {
    label: "Overview",
    eyebrow: "00 / Demo milestone",
    title: "Demo reached. The system remains default-off.",
    description:
      "The operator accepted the bounded Demo milestone on 2026-08-30. This screen remains a static repository view: it does not connect to a provider, account, market feed, or execution path.",
  },
  "system-map": {
    label: "System map",
    eyebrow: "01 / Ownership boundary",
    title: "Mynyra is the product home.",
    description:
      "The preserved engine lineage now lives under engine/. Credentials, account identifiers, provider traces, and generated runtime evidence are intentionally excluded.",
  },
  evidence: {
    label: "Evidence",
    eyebrow: "02 / Verification register",
    title: "Every future claim needs a named source and epoch.",
    description:
      "Repository checks and operator acceptance are named separately. This surface does not manufacture current account state, quotes, positions, orders, returns, or operational readiness.",
  },
};

const navigation = [
  { key: "overview" as const, label: "Overview", icon: Compass },
  { key: "system-map" as const, label: "System map", icon: Network },
  { key: "evidence" as const, label: "Evidence", icon: ClipboardCheck },
];

const handleFoundationAction = (action: string) => {
  toast.message(`${action} is not connected.`, {
    description:
      "This offline foundation contains no provider, credential, account, order, or deployment capability.",
  });
};

function EvidencePill({ children }: { children: React.ReactNode }) {
  return <span className="evidence-pill">{children}</span>;
}

function StatusDot({ tone = "quiet" }: { tone?: "quiet" | "copper" | "warn" }) {
  return <span className={`status-dot status-dot--${tone}`} aria-hidden="true" />;
}

export default function Home() {
  const [activePanel, setActivePanel] = useState<PanelKey>("overview");
  const panel = panels[activePanel];

  return (
    <div className="control-room-shell">
      <aside className="instrument-rail" aria-label="Control room navigation">
        <div className="rail-brand">
          <div className="brand-emblem">
            <span className="brand-fallback-mark" aria-hidden="true" />
          </div>
          <div>
            <p className="wordmark">Mynyra</p>
            <p className="wordmark-subtitle">Control room</p>
          </div>
        </div>

        <div className="rail-rule" />

        <nav className="rail-navigation" aria-label="Primary">
          <p className="nav-kicker">Reading frame</p>
          {navigation.map((item, index) => {
            const Icon = item.icon;
            const isActive = activePanel === item.key;
            return (
              <button
                key={item.key}
                type="button"
                className={`rail-nav-item ${isActive ? "is-active" : ""}`}
                onClick={() => setActivePanel(item.key)}
                aria-current={isActive ? "page" : undefined}
              >
                <span className="nav-index">0{index + 1}</span>
                <Icon size={15} strokeWidth={1.65} />
                <span>{item.label}</span>
              </button>
            );
          })}
        </nav>

        <div className="rail-spacer" />

        <section className="rail-boundary" aria-labelledby="boundary-title">
          <div className="boundary-icon">
            <LockKeyhole size={16} strokeWidth={1.7} />
          </div>
          <p id="boundary-title" className="boundary-label">
            Operating boundary
          </p>
          <p className="boundary-copy">
            Static view. Demo-capable source remains default-off.
          </p>
          <button
            type="button"
            className="quiet-link"
            onClick={() => handleFoundationAction("Boundary inspection")}
          >
            Read boundary <ArrowUpRight size={13} />
          </button>
        </section>

        <p className="rail-footnote">v1.0 · Demo milestone view</p>
      </aside>

      <main className="evidence-canvas">
        <header className="topline">
          <div className="topline-context">
            <span className="context-dot" />
          <span>Demo milestone</span>
            <span className="context-divider">/</span>
          <span>Static repository state</span>
          </div>
          <div className="topline-state">
            <StatusDot tone="copper" />
            <span>Default-off posture</span>
          </div>
        </header>

        <section className="safety-strip" aria-label="Safety notice">
          <div className="safety-strip-icon">
            <ShieldCheck size={18} strokeWidth={1.75} />
          </div>
          <p>
            <strong>No provider connection is active.</strong> The repository contains
            a compile-time gated Demo adapter, but this surface does not read credentials,
            contact providers, retrieve market data, or create orders.
          </p>
          <EvidencePill>STATIC VIEW</EvidencePill>
        </section>

        <section className="hero-readout" aria-labelledby="readout-title">
          <div className="hero-art" aria-hidden="true" />
          <div className="hero-overlay" />
          <div className="registration registration--top" aria-hidden="true">
            <span />
            <span />
          </div>
          <div className="hero-content">
            <p className="eyebrow">{panel.eyebrow}</p>
            <h1 id="readout-title">{panel.title}</h1>
            <p className="hero-description">{panel.description}</p>
            <div className="hero-actions">
              <button
                type="button"
                className="primary-action"
                onClick={() => handleFoundationAction("System inspection")}
              >
                Inspect local boundary <ChevronRight size={15} />
              </button>
              <button
                type="button"
                className="secondary-action"
                onClick={() => handleFoundationAction("Handoff view")}
              >
                View handoff structure
              </button>
            </div>
          </div>
          <div className="hero-meta" aria-label="Foundation status">
            <div>
              <p>Source posture</p>
              <strong>Consolidated repository</strong>
            </div>
            <div>
              <p>External state</p>
              <strong>Not observed</strong>
            </div>
          </div>
        </section>

        <section className="posture-section" aria-labelledby="posture-title">
          <div className="section-intro">
            <div>
              <p className="eyebrow eyebrow--ink">Current posture</p>
              <h2 id="posture-title">The controls that are intentionally absent.</h2>
            </div>
            <p>
              The first useful control room makes absence auditable. These are static
              declarations of the current product boundary, not live diagnostics.
            </p>
          </div>

          <div className="posture-ledger">
            <article className="posture-entry">
              <div className="posture-marker"><StatusDot tone="copper" /></div>
              <div>
                <p className="posture-label">Provider transport</p>
                <h3>Present but default-off</h3>
                <p>The Demo adapter is compile-time gated and this control-room surface has no connection path.</p>
              </div>
              <EvidencePill>GATED BY DESIGN</EvidencePill>
            </article>
            <article className="posture-entry">
              <div className="posture-marker"><StatusDot /></div>
              <div>
                <p className="posture-label">Financial account state</p>
                <h3>Not represented</h3>
                <p>No balances, positions, P&amp;L, quotes, symbols, or account identifiers are stored.</p>
              </div>
              <EvidencePill>NO DATA MODEL</EvidencePill>
            </article>
            <article className="posture-entry">
              <div className="posture-marker"><StatusDot tone="warn" /></div>
              <div>
                <p className="posture-label">Execution capability</p>
                <h3>Demo-bounded</h3>
                <p>The engine owns the canonical order path; LIVE support and UI-triggered execution remain absent.</p>
              </div>
              <EvidencePill>NO UI ACTION PATH</EvidencePill>
            </article>
          </div>
        </section>

        <section className="split-zone" aria-label="System and evidence panels">
          <article className="system-card">
            <div className="system-card-art" aria-hidden="true" />
            <div className="system-card-shade" />
            <div className="system-card-content">
              <div className="section-heading-inline">
                <div>
                  <p className="eyebrow">Ownership boundary</p>
                  <h2>The lineage is now one product tree.</h2>
                </div>
                <GitBranch size={19} strokeWidth={1.6} />
              </div>
              <div className="system-steps">
                <div className="system-step is-current">
                  <span>01</span>
                  <div>
                    <strong>Mynyra product repository</strong>
                    <p>GitHub-governed home for the control room, engine, evidence, and collaboration model.</p>
                  </div>
                </div>
                <div className="system-connector" />
                <div className="system-step">
                  <span>02</span>
                  <div>
                    <strong>Mynyra Engine</strong>
                    <p>History-preserving engine source under engine/ with default-off provider boundaries.</p>
                  </div>
                </div>
                <div className="system-connector system-connector--dashed" />
                <div className="system-step is-muted">
                  <span>03</span>
                  <div>
                    <strong>Methodical expansion</strong>
                    <p>New collaborators and capabilities receive named roles, review boundaries, and explicit authority.</p>
                  </div>
                </div>
              </div>
            </div>
          </article>

          <article className="evidence-card">
            <div className="evidence-art" aria-hidden="true" />
            <div className="evidence-card-content">
              <div className="section-heading-inline">
                <div>
                  <p className="eyebrow eyebrow--ink">Evidence register</p>
                  <h2>Claims need an address.</h2>
                </div>
                <Archive size={19} strokeWidth={1.6} />
              </div>
              <dl className="evidence-list">
                <div>
                  <dt>Interface state</dt>
                  <dd><StatusDot tone="copper" /> Static repository view</dd>
                </div>
                <div>
                  <dt>Demo milestone</dt>
                  <dd><StatusDot tone="copper" /> Operator accepted 2026-08-30</dd>
                </div>
                <div>
                  <dt>Current runtime state</dt>
                  <dd><StatusDot tone="warn" /> Not observed by this UI</dd>
                </div>
              </dl>
              <button
                type="button"
                className="text-action"
                onClick={() => setActivePanel("evidence")}
              >
                Open evidence reading frame <ChevronRight size={14} />
              </button>
            </div>
          </article>
        </section>

        <section className="handoff-band" aria-labelledby="handoff-title">
          <div className="handoff-art" aria-hidden="true" />
          <div className="handoff-shade" />
          <div className="handoff-copy">
            <p className="eyebrow">Continuation package</p>
            <h2 id="handoff-title">Built for a deliberate next reader.</h2>
            <p>
              The repository handoff separates scope, architecture, static data,
              verification, and prohibited actions so a later Codex pass can resume
              without inferring authority.
            </p>
          </div>
          <div className="handoff-files" aria-label="Handoff files">
            <div><FileText size={17} /><span>README.md</span></div>
            <div><Box size={17} /><span>ARCHITECTURE.md</span></div>
            <div><BookOpenCheck size={17} /><span>HANDOFF.md</span></div>
          </div>
        </section>

        <footer className="canvas-footer">
          <span>MYNYRA / DEMO MILESTONE</span>
          <span>Default-off · Evidence-aware · No live authority</span>
          <span><Sparkles size={13} /> Intent is visible.</span>
        </footer>
      </main>
    </div>
  );
}
