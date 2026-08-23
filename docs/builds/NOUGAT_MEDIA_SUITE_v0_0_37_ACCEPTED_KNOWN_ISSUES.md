# Nougat Media Suite v0.0.37 — Accepted State

Nougat Media Suite v0.0.37 is accepted as the current release.

Acceptance does not mean bug-free. Known issues and approved follow-up work are carried forward to v0.0.38.

## Validated v0.0.37 Milestones

- Native Linux ATSC Live TV playback works on real tuner hardware.
- Multiple OTA channels and subchannels were owner-tested successfully.
- Channel tuning is fast, clear and steady when RF reception is adequate.
- No unnecessary transcoding stage is used for normal Live TV playback.
- 30-second Live TV pause/resume was tested and resumed from the paused position.
- Existing ATSC scan functionality remains operational.
- Live TV channel selection, Watch Live and classic guide groundwork are present.
- Debug was renamed to System.
- Server Start / Stop / Refresh controls were moved from Library to System.
- Continue Watching card sizing was improved.
- Wide seek-bar work and stitched Server status indicator were added.

## Repository History Note

v0.0.36 was owner-tested and accepted but was not separately committed/tagged before v0.0.37 development began.

Do not fabricate a retroactive v0.0.36 Git commit.

v0.0.37 therefore becomes the next committed release after v0.0.35 and contains the cumulative accepted v0.0.36 and v0.0.37 work.

## v0.0.38 Agenda

### Library Search
- Clicking the Library Search field must focus it.
- Show a visible text caret.
- Typed characters must render immediately.
- "Search" is placeholder-only and must never become the query itself.
- Enter and the Search button execute the entered search.
- Empty search restores the full selected Library view.

### Library Media Cards
- Prioritize the complete movie / episode title in visible card space.
- Allow title wrapping where practical.
- Remove resolution / codec / audio clutter from the normal card text.
- Show technical information such as 1080p, 4K, H.264, H.265, AV1, AAC, AC3, DTS, container and runtime in a Nougat-style hover/focus information popup.
- If a title still cannot fit, the popup shows the complete untruncated title.

### Player Mouse Activity
- Pointer and media title/info overlay share one activity timer.
- Mouse movement shows both simultaneously.
- Continued movement resets the timer.
- Both remain visible for three seconds after mouse movement stops.
- Both disappear together.
- Behavior must be identical in fullscreen, maximized, normal and resized/half-screen playback.

### Player Seek / Volume Presentation
- Remove the redundant VOLUME label.
- Preserve the approved sheet seek and volume components.
- Remove the apparent pale/white rectangular matte surrounding them.
- The tan quilted player background must continue underneath the components.
- Controls sit directly over the background with transparent pixels outside their intended geometry.
- Preserve elapsed/total time and volume percentage.

### Home and Library Loading
- Home loading becomes determinate rather than continuously rolling.
- Home and Library both use the exact approved sheet component labeled PROGRESS BAR.
- Progress represents real completed loading work from 0% through 100%.
- Percentage text travels continuously with the leading edge of the caramel fill.
- Clamp the percentage inward near 0% and 100% so text never clips.

### Live TV Channels
- Up / Down navigates the channel list while that list has focus.
- Selected row receives a clear Nougat highlight.
- Enter tunes the selected channel.
- Mouse single-click selects.
- Mouse double-click tunes.
- Remember the last selected/current channel.
- Add a station-logo slot before each channel.
- Cache logos when available.
- Use a Nougat-styled numbered channel badge when no logo exists.

### Live TV Player Information
- Mouse activity overlay shows channel number, station name and current program.
- Use PSIP guide details when available.
- If guide information is unavailable, still show channel identity and Live TV status.

### Live TV Program Timing
- Keep elapsed program time on the left and total program duration on the right.
- Add actual program start clock time beneath the left value.
- Add actual program end clock time beneath the right value.
- Left values share one exact left anchor.
- Right values share one exact right anchor.
- Right-side strings must be positioned using rendered text width so their right edges align pixel-perfectly.

### Live TV Timeshift Characterization
- Current Live TV path successfully retained at least 30 seconds of paused broadcast data.
- Do not alter the working playback pipeline merely to add timeshift.
- Characterize current VLC buffering further before implementing a dedicated ring-buffer DVR/timeshift system.

### Installer Validation
- v0.0.37 installer printed an APPLY_COMMAND.txt baseline mismatch as FAIL and then subsequently passed the preflight.
- Repair validation logic so a FAIL cannot coexist with a successful result for the same gate.
