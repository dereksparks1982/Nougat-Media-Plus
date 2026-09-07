#include "../src/radio/v4l2_radio_provider.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>

int main() {
    using reddmedia::V4l2RadioProvider;

    // V4L2_TUNER_CAP_LOW uses 62.5 Hz units.
    const std::uint32_t low_100_1 = V4l2RadioProvider::frequency_units_from_hz(100100000.0, true);
    assert(low_100_1 == 1601600U);
    assert(std::fabs(V4l2RadioProvider::hz_from_frequency_units(low_100_1, true) - 100100000.0) < 1.0);

    // Legacy V4L2 radio units use 62.5 kHz units.
    const std::uint32_t legacy_100_1 = V4l2RadioProvider::frequency_units_from_hz(100100000.0, false);
    assert(legacy_100_1 == 1602U);
    assert(std::fabs(V4l2RadioProvider::hz_from_frequency_units(legacy_100_1, false) - 100125000.0) < 1.0);

    // US FM edges must be representable in LOW mode without truncation.
    assert(V4l2RadioProvider::frequency_units_from_hz(87500000.0, true) == 1400000U);
    assert(V4l2RadioProvider::frequency_units_from_hz(108000000.0, true) == 1728000U);

    assert(V4l2RadioProvider::frequency_units_from_hz(0.0, true) == 0U);
    assert(V4l2RadioProvider::frequency_units_from_hz(-1.0, true) == 0U);

    std::cout << "PASS: V4L2 radio frequency conversion\n";
    return 0;
}
