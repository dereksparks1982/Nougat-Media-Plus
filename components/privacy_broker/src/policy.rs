#[derive(Debug, Clone, Copy)]
pub struct PrivacyPolicy {
    pub direct_fallback: bool,
    pub plaintext_dns: bool,
    pub query_logging: bool,
}

impl PrivacyPolicy {
    pub const fn secure_search_baseline() -> Self {
        Self { direct_fallback: false, plaintext_dns: false, query_logging: false }
    }

    pub const fn valid(self) -> bool {
        !self.direct_fallback && !self.plaintext_dns && !self.query_logging
    }
}
