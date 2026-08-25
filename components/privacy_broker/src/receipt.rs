#[derive(Debug, Clone)]
pub struct PrivacyReceipt {
    pub network_query: bool,
    pub direct_fallback: bool,
    pub query_logged: bool,
}

impl PrivacyReceipt {
    pub const fn local_search() -> Self {
        Self { network_query: false, direct_fallback: false, query_logged: false }
    }
}
