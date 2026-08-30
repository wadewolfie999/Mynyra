# Mynyra Trade Control Room — Design Direction

## Three candidate approaches

| Theme Name | Very Brief Intro | Probability |
| --- | --- | --- |
| Alpine Ledger | A pale, archival interface with mineral paper textures and disciplined redaction marks. It would feel like an internal research folio. | 0.07 |
| Night Operations Manual | A dark, editorial control surface that uses exact typography, evidence rails, and copper registration marks to make system state legible without theatricality. | 0.04 |
| Signal Garden | A warm, organic dashboard that treats systems as a cultivated ecology, using botanical diagrams and restrained color fields. | 0.09 |

## Chosen approach: Night Operations Manual

### Design Movement

**Night Operations Manual** combines Swiss information design, technical field manuals, and contemporary editorial software surfaces. It avoids both speculative-market aesthetics and generic SaaS dashboard decoration.

### Core Principles

1. **Evidence before assertion.** Every state label distinguishes an observed fact from an inferred or unconfigured condition.
2. **Calm operational density.** Information is compact but breathable, with a persistent left rail and an asymmetric reading rhythm rather than a centered card grid.
3. **Boundaries are visible.** Offline-only and non-trading constraints are treated as first-class interface content, not footnotes.
4. **Product foundation, not a simulation.** The screen shows deliberately local, static placeholders; it does not invent brokerage status, account data, quotes, orders, or performance.

### Color Philosophy

The base is **midnight ink** so the interface reads as an instrument panel rather than a marketing site. Warm **burnished signal copper** identifies attention, boundaries, and deliberate choices—not positive performance. Ivory and slate provide legibility. Red is reserved only for explicit non-authorization language, while muted blue-gray supports inactive structure.

### Layout Paradigm

The page is a **field desk**: a fixed, narrow instrument rail on the left; a wide evidence canvas on the right; and vertical stacks that place current posture before future modules. On small screens, the rail resolves into a compact product strip. The main canvas uses uneven columns and rule lines rather than a uniform grid of rounded cards.

### Signature Elements

1. **Registration marks:** small copper brackets and dots that anchor important state blocks.
2. **Evidence rails:** hairline rules, sequence labels, and provenance chips that classify content as local, planned, or absent.
3. **Quiet texture fields:** bespoke midnight imagery used once per major area, not repeated as generic wallpaper.

### Interaction Philosophy

Interactions should clarify structure without implying operational capability. Tabs may change the local reading frame, while any unavailable control must state that it is a foundation-only placeholder. No buttons may suggest live connection, order routing, account access, or financial action.

### Animation

Use short opacity and transform transitions only. Initial sections may reveal in 180–260 ms with a 40–60 ms stagger; hover states may lift by one pixel and sharpen borders. Respect reduced-motion preferences. Never animate data values or imitate live market movement.

### Typography System

Use **Georgia / Iowan Old Style** for major editorial headings and **SFMono-Regular / Consolas / Liberation Mono** for states, labels, timestamps, and code-like provenance. Headings should be compact and high contrast; operational labels should use uppercase tracking and remain readable at small sizes. No Inter.

### Brand Essence

**Mynyra is a product-level command surface for building and governing trading-adjacent software with visible boundaries and auditable state.**

Personality: **disciplined, lucid, restrained**.

### Brand Voice

Headlines are direct, evidence-aware, and unembellished. CTAs identify the exact missing capability rather than promising progress.

> "Observe the system. Do not infer the market."

> "No provider connection is configured in this foundation."

### Wordmark & Logo

The wordmark should use a deliberate editorial serif with a custom letter-spaced treatment. The graphic mark is an interlocking compass-and-signal aperture: it represents orientation, observability, and constrained action without referencing finance.

### Signature Brand Color

**Burnished Signal Copper — `#C78F5B`**

## Implementation boundary

This foundation is intentionally **offline and non-trading**. It contains static local presentation data only. It must not contain credentials, provider artifacts, live provider calls, order capability, account identifiers, quotes, balances, performance claims, deployment logic, or implied operating authorization.
