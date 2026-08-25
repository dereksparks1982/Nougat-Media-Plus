#[derive(Debug, Clone)]
pub struct RelayDescriptor {
    pub operator: String,
    pub endpoint: String,
    pub autonomous_system: Option<u32>,
}

pub fn select_route(_relays: &[RelayDescriptor]) -> Option<Vec<RelayDescriptor>> {
    // v0.0.45 deliberately provides no remote route. Future versions can
    // replace this module without changing the broker protocol boundary.
    None
}
