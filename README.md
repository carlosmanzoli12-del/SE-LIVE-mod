# StreamForge for OBS Studio 30.0.2

An independently branded OBS Studio extension project. It is a clean-room implementation and does not reuse StreamElements source code, assets, trademarks, or credentials.

## Included in v0.1.0

- Native OBS dock and Tools-menu entry.
- Live stream and recording status, with start/stop controls.
- Replay Buffer save action (when Replay Buffer is enabled in OBS).
- Scene collection browser and current-scene switching.
- Source visibility toggling from the dock.
- Add a URL as an OBS Browser Source in the current scene.
- Initial workspace panels for destinations, unified chat, activity, overlays, and settings.
- GitHub Actions workflow that checks out OBS Studio **30.0.2**, registers the plugin in its CMake build, compiles the module, and uploads a Windows x64 ZIP artifact.

## Important scope note

This is a working foundation, not yet a feature-complete replacement for SE.Live. In v0.1.0, the Destinations and Unified Chat panels are scaffolding: they do not yet create independent RTMP outputs or authenticate Twitch/YouTube/TikTok. Those capabilities require platform-specific OAuth applications, API permissions, transport/output lifecycle management, and separate integration testing. No stream keys are collected or transmitted by this build.

## Build with GitHub Actions

1. Create a new GitHub repository named `streamforge` (or another name).
2. Upload the contents of this folder to the repository root.
3. Open **Actions** and enable workflows if GitHub asks.
4. Run **Build StreamForge for OBS 30.0.2** using **Run workflow**.
5. Download the artifact `StreamForge-OBS-30.0.2-Windows-x64` from the successful run.

The workflow compiles against the OBS Studio 30.0.2 source tree. A successful GitHub run is the final compatibility/build check; this environment has not executed that Windows build.

## Install (after a successful build)

Extract the ZIP into the OBS plugin data directory so the result is:

```text
%APPDATA%\obs-studio\plugins\streamforge\
  bin\64bit\streamforge.dll
  data\locale\en-US.ini
```

Restart OBS. Open **Docks → StreamForge** or **Tools → Open StreamForge**.

## Development

- Target OBS: 30.0.2 (Windows x64 CI target)
- Language: C++17
- UI: Qt 6 Widgets
- Build: CMake, integrated into OBS's own CMake tree

See [ROADMAP.md](ROADMAP.md) for planned integration work.
