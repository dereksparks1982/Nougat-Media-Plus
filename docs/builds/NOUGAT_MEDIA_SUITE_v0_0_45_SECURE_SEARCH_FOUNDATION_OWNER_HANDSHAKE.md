# Nougat Media Suite v0.0.45 Owner Handshake

## Build title
Secure Search Foundation, Privacy Broker & Crawler Access Architecture

## Approved scope
- Separate local Search from crawler networking.
- Remove silent DuckDuckGo/live-discovery Search fallback.
- Keep plaintext Search queries out of argv, URLs, environment variables, ordinary logs, diagnostics, and privacy receipts.
- Add versioned Secure Search, Privacy Policy, Privacy Receipt, Privacy Broker, and Crawler Access interfaces.
- Fail closed when secure remote Search is unavailable. Never direct-fallback.
- Use truthful `NougatSearchCrawler` identity for indexing.
- Respect robots policy and classify blocked/rate-limited/auth/payment/unavailable crawler states.
- Bind legacy administrative node service to loopback only and disable plaintext remote Search endpoint.
- Add Rust Privacy Broker scaffold without making Rust a required v0.0.45 application build dependency.
- Keep Secure Search implementation outside `src/main.cpp`; main contains wiring only.

## Deferred behind versioned interfaces
Production OHTTP/multi-relay transport, ODoH, ECH integration, PIR/HE query-private retrieval, mix/batching/cover traffic, browser renderer containment, signed relay directory, and production post-quantum transport.

Candidate remains uncommitted/untagged/unpushed until owner acceptance.
