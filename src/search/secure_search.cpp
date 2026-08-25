#include "secure_search.hpp"

#include "../privacy/privacy_broker_client.hpp"
#include "../privacy/privacy_policy.hpp"
#include "../privacy/privacy_receipt.hpp"

namespace reddmedia {

const char* SecureSearchController::stage_label(SecureSearchStage stage) {
    switch (stage) {
    case SecureSearchStage::Idle: return "Secure Search ready.";
    case SecureSearchStage::SearchingLocalIndex: return "Searching Local Index...";
    case SecureSearchStage::SecuringRoute: return "Securing Route...";
    case SecureSearchStage::EncryptingQuery: return "Encrypting Query...";
    case SecureSearchStage::SearchingSources: return "Searching Sources...";
    case SecureSearchStage::VerifyingResults: return "Verifying Results...";
    case SecureSearchStage::Complete: return "Secure Search Complete";
    case SecureSearchStage::Unavailable: return "Secure Search unavailable. Search not sent.";
    case SecureSearchStage::Failed: return "Secure Search failed.";
    }
    return "Secure Search failed.";
}

NougatSearchResponse SecureSearchController::search(NougatBridge& bridge,
                                                    const std::string& query,
                                                    bool raw,
                                                    bool request_remote,
                                                    int limit,
                                                    int offset) {
    NougatSearchResponse response;
    const PrivacyPolicy policy = PrivacyPolicy::secure_search_baseline();
    std::string policy_error;
    if (!policy.valid(policy_error)) {
        response.error = policy_error;
        response.privacy_stage = stage_label(SecureSearchStage::Failed);
        return response;
    }

    response = bridge.search(query, raw, false, limit, offset);
    if (!response.error.empty()) {
        response.privacy_stage = stage_label(SecureSearchStage::Failed);
        return response;
    }

    PrivacyReceipt receipt;
    response.privacy_stage = stage_label(SecureSearchStage::Complete);

    if (request_remote) {
        const PrivacyBrokerStatus broker = PrivacyBrokerClient().status();
        const std::string detail = broker.remote_search_ready
            ? "Secure remote query transport is not enabled in v0.0.45; search remained local."
            : "Secure remote search unavailable. Search was not sent outside this device.";
        response.peer_status.emplace_back("SECURE-REMOTE", detail);
        response.secure_remote_unavailable = true;
        // Deliberately do not pass the query to the broker in v0.0.45.
    }

    response.privacy_receipt = receipt.to_text();
    return response;
}

} // namespace reddmedia
