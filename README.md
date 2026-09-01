# Resize Symmetrically

A small native Windows 11 utility that resizes desktop windows symmetrically around their center.

## Usage

1. Run `ResizeSymmetrically.exe`.
2. Hold the configured modifier key (Alt by default).
3. Drag a resizable window edge or corner.

Releasing the modifier pauses the current drag. Pressing it again resumes from the original gesture; releasing the left mouse button finishes the gesture.

The tray menu opens settings or exits the utility. Settings include per-user startup, modifier selection, and Russian/English UI language.

Elevated, maximized, fullscreen, non-resizable, and system windows are intentionally ignored.

## Build

Requirements:

- Visual Studio Build Tools with MSVC x64 and the Windows 11 SDK
- Inno Setup 6 only when building the installer

```powershell
.\build.ps1 -Configuration Release
.\build.ps1 -Configuration Release -Installer
```

Artifacts are written to `bin\x64\Release` and `artifacts`.

`--exit` is available for build and installer smoke tests; it closes an existing instance without opening the settings window.
