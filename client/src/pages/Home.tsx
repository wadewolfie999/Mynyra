/**
 * Design: Night Operations Manual — an offline field desk with evidence rails,
 * visible system boundaries, editorial hierarchy, and no implied live capability.
 */
import { useState, type SyntheticEvent } from "react";
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
    eyebrow: "00 / Local posture",
    title: "A product shell with its boundaries intact.",
    description:
      "This screen is a static implementation foundation. It records intent and interface structure without connecting to a provider, account, market feed, or execution path.",
  },
  "system-map": {
    label: "System map",
    eyebrow: "01 / Ownership boundary",
    title: "Mynyra is being shaped as the product home.",
    description:
      "TradeBot remains a separate source repository during this transition. No files, credentials, provider artifacts, or runtime behavior have been imported into this product shell.",
  },
  evidence: {
    label: "Evidence",
    eyebrow: "02 / Verification register",
    title: "Every future claim needs a named source and epoch.",
    description:
      "The foundation provides a place to classify evidence. It does not manufacture account state, quotes, positions, orders, returns, or operational readiness.",
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

const hideManagedAssetOnError = (event: SyntheticEvent<HTMLImageElement>) => {
  event.currentTarget.style.display = "none";
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
            <img
              src="/manus-storage/mynyra-logo_e85fd754.png"
              alt="Mynyra brand mark"
              className="brand-mark"
              onError={hideManagedAssetOnError}
            />
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
            Offline-only foundation. No connection surface is present.
          </p>
          <button
            type="button"
            className="quiet-link"
            onClick={() => handleFoundationAction("Boundary inspection")}
          >
            Read boundary <ArrowUpRight size={13} />
          </button>
        </section>

        <p className="rail-footnote">v0.1 · local interface scaffold</p>
      </aside>

      <main className="evidence-canvas">
        <header className="topline">
          <div className="topline-context">
            <span className="context-dot" />
            <span>Product foundation</span>
            <span className="context-divider">/</span>
            <span>Static local state</span>
          </div>
          <div className="topline-state">
            <StatusDot tone="copper" />
            <span>Non-trading mode</span>
          </div>
        </header>

        <section className="safety-strip" aria-label="Safety notice">
          <div className="safety-strip-icon">
            <ShieldCheck size={18} strokeWidth={1.75} />
          </div>
          <p>
            <strong>No provider connection is configured.</strong> This surface holds
            product structure only; it does not read credentials, contact providers,
            retrieve market data, or create orders.
          </p>
          <EvidencePill>LOCAL SCAFFOLD</EvidencePill>
        </section>

        <section className="hero-readout" aria-labelledby="readout-title">
          <img
            src="/manus-storage/mynyra-console-grid_7710f53f.jpg"
            alt="Abstract midnight systems topology"
            className="hero-art"
            onError={hideManagedAssetOnError}
          />
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
              <strong>Standalone shell</strong>
            </div>
            <div>
              <p>External state</p>
              <strong>Not attached</strong>
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
                <h3>Not configured</h3>
                <p>No API client, network call, callback, or provider artifact is included.</p>
              </div>
              <EvidencePill>ABSENT BY DESIGN</EvidencePill>
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
                <h3>Prohibited</h3>
                <p>No order workflow, paper endpoint, live switch, or risk-limit mutation exists here.</p>
              </div>
              <EvidencePill>NO ACTION PATH</EvidencePill>
            </article>
          </div>
        </section>

        <section className="split-zone" aria-label="System and evidence panels">
          <article className="system-card">
            <img
              src="/manus-storage/mynyra-architecture-schematic_b264a119.jpg"
              alt="Abstract architectural pathways"
              className="system-card-art"
              onError={hideManagedAssetOnError}
            />
            <div className="system-card-shade" />
            <div className="system-card-content">
              <div className="section-heading-inline">
                <div>
                  <p className="eyebrow">Ownership boundary</p>
                  <h2>Source is not yet product.</h2>
                </div>
                <GitBranch size={19} strokeWidth={1.6} />
              </div>
              <div className="system-steps">
                <div className="system-step is-current">
                  <span>01</span>
                  <div>
                    <strong>Mynyra product shell</strong>
                    <p>Current workspace for the future software-level control surface.</p>
                  </div>
                </div>
                <div className="system-connector" />
                <div className="system-step">
                  <span>02</span>
                  <div>
                    <strong>TradeBot source repository</strong>
                    <p>Existing implementation input; not copied or attached by this foundation.</p>
                  </div>
                </div>
                <div className="system-connector system-connector--dashed" />
                <div className="system-step is-muted">
                  <span>03</span>
                  <div>
                    <strong>Future governed integration</strong>
                    <p>Requires a separately approved product, evidence, and risk design.</p>
                  </div>
                </div>
              </div>
            </div>
          </article>

          <article className="evidence-card">
            <img
              src="/manus-storage/mynyra-evidence-topography_6459bc0f.jpg"
              alt="Abstract archival evidence texture"
              className="evidence-art"
              onError={hideManagedAssetOnError}
            />
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
                  <dd><StatusDot tone="copper" /> Static local copy</dd>
                </div>
                <div>
                  <dt>Provider evidence</dt>
                  <dd><StatusDot /> Not imported</dd>
                </div>
                <div>
                  <dt>Execution evidence</dt>
                  <dd><StatusDot tone="warn" /> Not authorized</dd>
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
          <img
            src="/manus-storage/mynyra-quiet-surface_1a17cd0c.jpg"
            alt="Abstract dark boundary texture"
            className="handoff-art"
            onError={hideManagedAssetOnError}
          />
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
          <span>MYNYRA / PRODUCT FOUNDATION</span>
          <span>Offline · Non-trading · No external state</span>
          <span><Sparkles size={13} /> Intent is visible.</span>
        </footer>
      </main>
    </div>
  );
}
