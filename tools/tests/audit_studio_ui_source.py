#!/usr/bin/env python3
"""Static Studio UI integration checks; does not run Nougat or access external media.

Existing approved Media Convert rectangles and tools geometry must not move.
This verifies integration wiring only, not graphical behavior.
"""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[2]
SOURCE=(ROOT/"src/main.cpp").read_text(encoding="utf-8")
SERVICE=(ROOT/"src/studio/studio_service.cpp").read_text(encoding="utf-8")

class StudioHistoryUiContract(unittest.TestCase):
    def test_media_convert_already_integrated(self):
        self.assertIn("studioPanel==StudioPanel::MediaConvert", SOURCE)
        self.assertIn("start_studio_convert_job()", SOURCE)
        self.assertIn("studioConvertSubmission = std::async(std::launch::async", SOURCE)
        self.assertIn("studioConvertService->cancel(studioConvertSelectedJob)", SOURCE)
        self.assertIn("studioConvertService->retry(studioConvertSelectedJob)", SOURCE)
    def test_scrolling_preserves_existing_controls(self):
        self.assertIn("studioConvertHistoryViewport", SOURCE)
        self.assertIn("studioConvertHistoryOffset", SOURCE)
        self.assertIn("studioConvertHistoryViewport.contains(x,y)", SOURCE)
        self.assertIn("ks == XK_Page_Up || ks == XK_Page_Down", SOURCE)
        self.assertIn("button_on(target,studioConvertCancelBtn,\"Cancel Job\")", SOURCE)
        self.assertIn("button_on(target,studioConvertRetryBtn,\"Retry Job\")", SOURCE)
        self.assertIn("panel.y+278+verticalShift", SOURCE)
        self.assertIn("panel.y+238+verticalShift", SOURCE)
        self.assertIn("panel.h-292-verticalShift", SOURCE)
    def test_diagnostic_and_history_commands(self):
        self.assertIn("studioConvertService->recent_log(item->id,4096U)", SOURCE)
        self.assertIn("studioConvertService->forget(item->id)", SOURCE)
        self.assertIn("own_clipboard_text(detail)", SOURCE)
        self.assertIn("Ctrl+Del forget", SOURCE)
        self.assertIn("Export and encoder log are preserved.", SOURCE)
        self.assertIn("engine_.recent_log(id, limit)", SERVICE)
        self.assertIn("engine_.forget(id)", SERVICE)
    def test_media_convert_file_browser_keyboard(self):
        studio_key=SOURCE[SOURCE.index("void handle_studio_key("):
                          SOURCE.index("void process_studio_worker_line(")]
        self.assertLess(studio_key.index("if (studio_browser_active())"),
                        studio_key.index("if (studioPanel != StudioPanel::FileSplitter)"))
        self.assertIn("studio_browser_apply_selection()", studio_key)
        self.assertIn("close_studio_browser()", studio_key)

if __name__ == "__main__":
    unittest.main(verbosity=2)
