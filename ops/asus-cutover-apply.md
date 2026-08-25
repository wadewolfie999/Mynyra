# ASUS Mynyra Cutover APPLY Envelope

This record governs only the Mynyra migration. It does not modify Node Control,
Tracker, SSH, containerd, either reverse-tunnel service, or Jellyfin.

## Scope

- Target: `asus-node` (`wolfski`), reached through `asus-remote`.
- Mutable paths: `/home/wade/Mynyra-Trade`,
  `/home/wade/Mynyra-Trade-asus-rid-z2egmTZp-recovery-20260826`, `/opt/mynyra-trade`,
  `/var/lib/mynyra-engine`, `/var/lib/mynyra-radicle`, and the two named Mynyra
  systemd units only.
- Accounts: `mynyra-offline`, `mynyra-control-room`, and `mynyra-radicle` only.
- Network: loopback TCP `3100` for the static Control Room; outbound-only
  Radicle replication; no public listener or `radicle-httpd`.

## Preconditions

Before each mutation, verify the administration route, free space, recovery
bundle, source revision, pinned tool hashes, current listeners, and the health
of Tracker, SSH, containerd, `reverse-ssh.service`, and
`asus-reverse-tunnel.service`. Stop if a protected workload differs from its
recorded state.

## Causal order

1. Preserve the unrelated ASUS repository by same-filesystem rename, then
   clone only the canonical Mynyra RID into `/home/wade/Mynyra-Trade`.
2. Build and verify the immutable offline release; run its transient,
   private-network replay proof and stop for acceptance.
3. Only after acceptance, install and start `mynyra-control-room.service`.

## Rollback

Stop and disable only new Mynyra units. Repoint `/opt/mynyra-trade/current` to
the prior release or remove that symlink, retain releases/evidence, and reverse
the ASUS repository rename only before a canonical clone is created. Do not
restart, modify, or remove protected workloads.
