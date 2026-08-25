#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TransportState {
    LocalOnly,
    RemoteUnavailable,
}

pub const fn current_transport_state() -> TransportState {
    TransportState::LocalOnly
}
