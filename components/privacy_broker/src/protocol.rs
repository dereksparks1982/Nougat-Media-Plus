pub const PROTOCOL_VERSION: u32 = 1;
pub const PROTOCOL_NAME: &str = "PrivacyBrokerProtocol/1";

pub fn handle_line(line: &str) -> &'static str {
    match line.trim() {
        "HELLO 1" => "OK PrivacyBrokerProtocol/1\n",
        "STATUS" => "OK local-only fail-closed remote-search-unavailable\n",
        _ => "UNAVAILABLE remote-query-transport-not-implemented\n",
    }
}
