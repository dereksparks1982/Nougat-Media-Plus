#!/usr/bin/env python3
"""No-network, source-only Games integration checks.

These checks deliberately do not spawn emulators, open X11 windows, require ROMs,
run GitHub Actions, or touch external storage. Passing does not prove gameplay.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
GAMES = ROOT / "src" / "games"
HOST = (GAMES / "emulator_host.cpp").read_text(encoding="utf-8")
POLICY = (GAMES / "emulator_window_policy.hpp").read_text(encoding="utf-8")
REGISTRY = (GAMES / "emulator_registry.cpp").read_text(encoding="utf-8")
REGISTRY_H = (GAMES / "emulator_registry.hpp").read_text(encoding="utf-8")
CMAKE = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")


class GamesSourceContract(unittest.TestCase):
    def test_embedding_and_owner_evidence(self):
        self.assertIn('#include "emulator_window_policy.hpp"', HOST)
        self.assertIn("window_policy::eligible(evidence)", HOST)
        self.assertIn("process_descends_from(owner_pid, child)", HOST)
        self.assertIn("getpgid(owner_pid) == process_group", HOST)
        self.assertIn("owner_pid > 1 && !process_owned", HOST)
        self.assertNotIn("fresh_strict_backend_window", HOST)
        self.assertIn("e.reports_other_process", POLICY)
        self.assertIn("e.backend_identity_matches", POLICY)
        self.assertIn("e.newly_seen", POLICY)
        self.assertIn("e.window_manager_client", POLICY)

    def test_game_ends_when_window_lost(self):
        self.assertIn("if (!impl_->embedded_window || !impl_->window_still_exists", HOST)
        self.assertIn("reap_child_until(child, std::chrono::milliseconds(750))", HOST)
        self.assertIn("(void)reap_child_until(child, std::chrono::milliseconds(1500))", HOST)
        self.assertIn("impl_->state = HostState::Exited;", HOST)
        self.assertIn("stop();\n            impl_->state = HostState::Exited;", HOST)
        self.assertIn("kill(-process_group, SIGTERM)", HOST)
        self.assertIn("kill(-process_group, SIGKILL)", HOST)

    def test_player_surface_is_authoritative(self):
        self.assertIn('setenv("NOUGAT_EMBED_XID", nougat_embed_xid.c_str(), 1)', HOST)
        self.assertIn("std::clamp(request.window_timeout_ms, 5000, 90000)", HOST)
        env_index = HOST.index("for (const auto& entry : request.environment)")
        embed_index = HOST.index('setenv("NOUGAT_EMBED_XID", nougat_embed_xid.c_str(), 1)')
        self.assertLess(env_index, embed_index)

    def test_registry_is_compiled_but_playability_not_exaggerated(self):
        self.assertIn("src/games/emulator_registry.cpp", CMAKE)
        self.assertIn("src/games/emulator_host.cpp", CMAKE)
        self.assertIn("inspect_emulation_support", REGISTRY)
        self.assertIn("inspect_emulation_support", REGISTRY_H)
        self.assertIn("DetectedUnverified", REGISTRY)
        self.assertIn("FirmwareCandidateMissing", REGISTRY)
        self.assertIn("launch_tested = false", REGISTRY)
        self.assertIn("containment_tested = false", REGISTRY)
        self.assertIn("getpgid", HOST)

    def test_test_sources_exist(self):
        self.assertTrue((ROOT / "tools/tests/test_emulator_window_policy.cpp").is_file())
        self.assertTrue((ROOT / "tools/tests/test_emulator_registry_probe.cpp").is_file())
        self.assertIn("static_assert", (ROOT / "tools/tests/test_emulator_window_policy.cpp").read_text())
        self.assertIn("inspect_emulation_support", (ROOT / "tools/tests/test_emulator_registry_probe.cpp").read_text())

    def test_no_release_version_or_geometry_change_here(self):
        self.assertIn('NOUGAT_V61_EMBED_WORKDIR_REMOVED', HOST)
        self.assertIn("XMoveResizeWindow(display, window, 0, 0,", HOST)
        self.assertIn("Nougat_Media_Plus_v68", CMAKE)  # existing build target only, NOT owner-approved update
        self.assertNotIn("Nougat_Media_Plus_v70", CMAKE)


if __name__ == "__main__":
    unittest.main(verbosity=2)
