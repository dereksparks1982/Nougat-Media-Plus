# Nougat Secure Search Architecture v1

Nougat Media Suite v0.0.45 establishes a fail-closed security boundary for Search.
Elderred Softworks LLC is the developer; Nougat Media Suite is the product.

## Privacy Law

1. A plaintext search query must not be placed in process argv, environment variables, URLs, ordinary logs, diagnostics, privacy receipts, or temporary files.
2. The local Search worker has no ordinary network client implementation.
3. Remote Search is disabled unless a Privacy Broker transport explicitly proves that the required privacy policy is available.
4. A failed or unavailable privacy path never falls back to direct Search networking.
5. Crawler activity is a separate security domain from user Search activity.
6. A crawler identity may be persistent for crawler administration, but it must never be attached to a user's Search request.
7. All privacy-critical interfaces are versioned so they can be replaced as standards and threats change.

## v0.0.45 boundaries

- `nougat_search_worker.py`: local SQLite FTS retrieval only; query arrives through stdin IPC.
- `nougat_crawler_worker.py`: public-web indexing only; receives crawler seed through stdin IPC and never receives a user Search query.
- `nougat_engine.py`: loopback-only administrative peer/node surface. The old plaintext URL Search endpoint is disabled.
- `SecureSearchController`: Search policy and fail-closed remote-request decision point.
- `PrivacyBrokerClient`: versioned Unix-domain-socket status interface. v0.0.45 does not send user queries to the broker.
- Rust Privacy Broker scaffold: `PrivacyBrokerProtocol/1`; local-only and fail-closed in this release.

## Versioned interfaces

- PrivacyBrokerProtocol/1
- PrivacyReceipt/v1
- PrivacyPolicy protocol version 1
- Crawler Access State v1

Future OHTTP, multi-relay, oblivious DNS, ECH, PIR, homomorphic retrieval, post-quantum algorithms, mix/batching transport, browser containment, and signed relay-directory implementations are expected to replace or extend providers behind these interfaces rather than rewrite Search.

## Threat-model truth

v0.0.45 materially reduces accidental query leakage and removes the prior silent live-discovery fallback. It does not yet claim anonymity against a global passive adversary, a compromised operating system, malicious firmware, or endpoint malware. Remote private retrieval is intentionally unavailable until the stronger transport exists.
