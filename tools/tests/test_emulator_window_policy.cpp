// Standalone C++17 unit test. Does not spawn emulators or require X11.
// Compile manually when source verification is authorized:
// c++ -std=c++17 -Wall -Wextra -Werror -pedantic
//     -Isrc tools/tests/test_emulator_window_policy.cpp -o /tmp/nougat-window-policy-test
// (Run the resulting test from the internal checkout; do not publish binaries.)
#include "games/emulator_window_policy.hpp"

using nougat::games::window_policy::Evidence;
using nougat::games::window_policy::eligible;

static_assert(!eligible(Evidence{}), "An arbitrary window must be rejected");
static_assert(eligible(Evidence{false, true}), "Already-contained child should be adopted");
static_assert(eligible(Evidence{true, false, true}), "Process descendant can be captured");
static_assert(eligible(Evidence{true, false, false, true}), "Same isolated process group is owned");
static_assert(eligible(Evidence{true, false, false, false, true}),
              "Matching unnamed-PID runtime is a narrowly permitted fallback");
static_assert(!eligible(Evidence{true}), "New size alone is not ownership");
static_assert(!eligible(Evidence{false, false, true}),
              "Preexisting windows never become game targets");
static_assert(!eligible(Evidence{true, false, false, false, true, true}),
              "Explicitly unrelated PID cannot be overridden by title");
static_assert(!eligible(Evidence{true, false, false, false, true, false, true, true, false}),
              "An unrelated Xenia toolkit child is not the window manager client");
static_assert(eligible(Evidence{true, false, false, false, true, false, true, true, true}),
              "Matching Xenia EWMH client can be selected");
static_assert(eligible(Evidence{true, false, true, false, false, false, true, true, false}),
              "Known-owned backend can be selected without a title match");

int main() { return 0; }
