# Isolated Nougat Studio core test project

This directory defines a **separate CMake project** for manually validating the source-only Studio engine, FFprobe inspector, and Studio service without Nougat's full UI, AI runtime, streaming dependencies, emulator stack or media artwork.

It uses standard C++17 and POSIX subprocesses. The test executables use only private temporary files and scripted fake FFprobe/FFmpeg stand-ins. They do not require media codecs, a network connection, user ROMs, user libraries or the external drive.

Tests: `test_studio_media_job_engine`, `test_studio_media_inspector` and `test_studio_fake_ffmpeg`. The root application build remains unchanged aside from Studio source integration. This test project **does not run automatically**, and there is deliberately **no GitHub Actions workflow**.

To protect the active development installation, any future CMake configure/build/CTest execution should use a disposable build directory **on the internal development drive**, never an in-source build or the external storage drive. Do not run this test project against a user's actual media folders.

Passing the isolated tests would not prove MP4/Matroska codec support, real output quality, the Studio GUI controls, cancellation under hardware load or libVLC playback. Those still require a separate acceptance phase.
