# ReddMedia Known Bugs

## v0.0.14 owner-test focus

- v0.0.13 owner testing found that a local video paused for several minutes could freeze the application and prevent normal close. v0.0.14 changes the local pause/poll/shutdown lifecycle and requires owner validation with long pauses before acceptance.

## Carried forward from accepted v0.0.13

## v0.0.13

- Actual YouTube playback resolution is not yet directly reported by ReddMedia and may appear below the intended 1080p target.
- Seeking may take approximately 10 to 15 seconds before playback resumes.
- Playback can enter repeated buffer/play cycles after seeking.

These items move to v0.0.14.
