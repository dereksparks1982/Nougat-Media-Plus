# Nougat Media Suite v0.0.45 Validation Contract

Required candidate gates:

- Full native build passes with warnings as errors.
- v0.0.44 retained features remain operational.
- Search query is absent from process argv and URL query strings.
- Search worker has no direct network client imports or remote discovery path.
- DuckDuckGo live-discovery fallback is absent.
- Plain HTTP peer Search is unavailable.
- Local Search operates entirely offline against the existing Nougat index.
- Secure remote request fails closed and explicitly records that the query was not sent.
- Persistent administrative node identity is never emitted by user Search output.
- Crawler and Search workers are separate executables/process domains.
- `NougatSearchCrawler/0.0.45` truthfully identifies search-index crawling.
- Crawler access classifications include robots restriction, bot-policy block, rate limit, authentication required, feed available, payment required, and temporary unavailability.
- Privacy Broker interface is versioned and does not enable remote query transport in v0.0.45.
- Privacy receipt contains no query text.
- Secure Search progress label describes actual local-index work only.
- No privacy/network implementation is added to `src/main.cpp` beyond Secure Search controller wiring.
