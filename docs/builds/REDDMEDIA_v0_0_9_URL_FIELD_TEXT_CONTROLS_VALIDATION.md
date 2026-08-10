# ReddMedia v0.0.9 URL Field Text Controls Validation

## Build-side gates

- Exact v0.0.8 Git base verified before candidate work.
- C++ source compiles under the project CMake configuration with `-Wall -Wextra -Werror`.
- `ReddMedia_v9 --version` prints exactly `ReddMedia v0.0.9`.
- `ReddMedia_v9` is a native ELF executable.
- Version surfaces use v0.0.9 and `ReddMedia_v9`.
- Ctrl+A key handling sets full URL-field selection state.
- URL-field right-click handling opens a three-item Cut / Copy / Paste popup.
- Cut, Copy, and Paste are connected to the existing menu-action dispatcher.
- Cut and Copy own the X11 CLIPBOARD selection and serve selection requests.
- Package inventory matches the changed-files-only manifest.
- Package payload and accepted-base hashes match the manifest.
- Installer rollback is tested on a disposable accepted-base clone.

## Owner-side live UI validation

1. Open `ReddMedia_v9` and switch to yt-dlp.
2. Put a URL in the URL field.
3. Press Ctrl+A and confirm the complete field value is selected/highlighted.
4. Right-click inside the URL field and confirm Cut / Copy / Paste appears.
5. Use Copy and confirm the URL remains in the field and is available to paste.
6. Press Ctrl+A, use Cut, and confirm the URL leaves the field and remains available to paste.
7. Use Paste and confirm the URL is inserted into the field.

## Build-side results before package sealing

- PASS: CMake configure/build with GNU C++ 14.2.0 and project `-Wall -Wextra -Werror`.
- PASS: `ReddMedia_v9 --version` returned exactly `ReddMedia v0.0.9`.
- PASS: `file ReddMedia_v9` identified a native x86-64 GNU/Linux ELF executable.
- PASS: X11 Xvfb interaction drove the real application window through Ctrl+A, right-click Copy, right-click Cut, and right-click Paste; clipboard roundtrip returned the complete typed field value `abcdefg`.

## Package and rollback rehearsal

- PASS: changed-files-only package inventory and payload hashes matched the manifest.
- PASS: installer verified exact v0.0.8 commit, tag, branch, clean tree, and accepted-base hashes before mutation.
- PASS: disposable clean v0.0.8 clone completed every installer gate and ended with `FINAL PASS: ReddMedia v0.0.9 URL Field Text Controls installed and validated`.
- PASS: forced post-mutation failure triggered installer rollback; Git HEAD returned to `21b6219fd57deb14d48815a47df4f2840ed04875`, the tree returned clean, `ReddMedia_v8` was restored, and candidate `ReddMedia_v9` was removed.
