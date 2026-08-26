#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
MAIN = ROOT / "src" / "main.cpp"
MARKER = "NOUGAT_V50_NEUTRAL_PICKERS"


def main() -> int:
    try:
        text = MAIN.read_text(encoding="utf-8")
        if MARKER in text:
            if "--title='Choose File'" not in text or "--title='Choose Folder'" not in text:
                raise RuntimeError("neutral-picker marker exists but expected picker labels are missing")
            print("PASS: v0.0.50 neutral picker patch already applied")
            return 0

        file_old = '''static std::string choose_file_dialog() {
    std::string p;
    p = run_command_capture("command -v zenity >/dev/null 2>&1 && zenity --file-selection --title='Open Media' 2>/dev/null");
    if (!p.empty()) return p;
    std::string py =
        "python3 -c \\"import tkinter as tk; from tkinter import filedialog; "
        "root=tk.Tk(); root.withdraw(); "
        "p=filedialog.askopenfilename(title='Open Media'); print(p if p else '')\\" 2>/dev/null";
    p = run_command_capture(py);
    return p;
}
'''
        file_new = '''// NOUGAT_V50_NEUTRAL_PICKERS
static std::string choose_file_dialog() {
    std::string p;
    p = run_command_capture("command -v zenity >/dev/null 2>&1 && zenity --file-selection --title='Choose File' 2>/dev/null");
    if (!p.empty()) return p;
    std::string py =
        "python3 -c \\"import tkinter as tk; from tkinter import filedialog; "
        "root=tk.Tk(); root.withdraw(); "
        "p=filedialog.askopenfilename(title='Choose File'); print(p if p else '')\\" 2>/dev/null";
    p = run_command_capture(py);
    return p;
}
'''
        folder_old = '''static std::string choose_folder_dialog() {
    std::string p;
    p = run_command_capture("command -v zenity >/dev/null 2>&1 && zenity --file-selection --directory --title='Open Subtitle Folder' 2>/dev/null");
    if (!p.empty()) return p;
    std::string py =
        "python3 -c \\"import tkinter as tk; from tkinter import filedialog; "
        "root=tk.Tk(); root.withdraw(); "
        "p=filedialog.askdirectory(title='Open Subtitle Folder'); print(p if p else '')\\" 2>/dev/null";
    return run_command_capture(py);
}
'''
        folder_new = '''static std::string choose_folder_dialog() {
    std::string p;
    p = run_command_capture("command -v zenity >/dev/null 2>&1 && zenity --file-selection --directory --title='Choose Folder' 2>/dev/null");
    if (!p.empty()) return p;
    std::string py =
        "python3 -c \\"import tkinter as tk; from tkinter import filedialog; "
        "root=tk.Tk(); root.withdraw(); "
        "p=filedialog.askdirectory(title='Choose Folder'); print(p if p else '')\\" 2>/dev/null";
    return run_command_capture(py);
}
'''
        if text.count(file_old) != 1:
            raise RuntimeError(f"generic file-picker base mismatch: found {text.count(file_old)} matches")
        if text.count(folder_old) != 1:
            raise RuntimeError(f"generic folder-picker base mismatch: found {text.count(folder_old)} matches")
        updated = text.replace(file_old, file_new, 1).replace(folder_old, folder_new, 1)
        if MARKER not in updated or "Open Media'" in updated or "askdirectory(title='Open Subtitle Folder')" in updated:
            raise RuntimeError("neutral picker final validation failed")
        MAIN.write_text(updated, encoding="utf-8")
        print("PASS: generic file/folder picker labels are neutral for Workshop and other features")
        return 0
    except Exception as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
