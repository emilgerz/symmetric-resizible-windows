# AGENTS.md

## Scope

These instructions apply to the entire repository.

Resize Symmetrically is a small native Windows 11 x64 utility written in C++20 and Win32. Keep it self-contained: do not introduce a managed runtime, background service, polling loop, or third-party runtime dependency without an explicit requirement.

## Repository layout

- `src/` — application, hooks, geometry, settings, localization, resources, and manifest.
- `tests/GeometryTests.cpp` — deterministic geometry unit tests.
- `tests/TestWindow.cpp` — manual Win32 integration window.
- `installer/ResizeSymmetrically.iss` — per-user Inno Setup installer.
- `tools/GenerateIcon.ps1` — regenerates the ICO from `assets/cross-cursor.svg`.
- `build.ps1` — canonical build and test entry point.

Generated directories (`bin/`, `obj/`, and `artifacts/`) are ignored and must not be committed.

## Build and test

Use the repository build script from PowerShell:

```powershell
.\build.ps1 -Configuration Release
.\build.ps1 -Configuration Release -Installer
```

The required toolchain is Visual Studio Build Tools with MSVC x64 and the Windows 11 SDK. Inno Setup 6 is required only for installer builds.

Do not finish a code change with failing geometry tests, compiler errors, or new compiler warnings. Add or update geometry tests whenever edge, corner, tracking-size, monitor-boundary, or negative-coordinate behavior changes.

## Behavioral invariants

- Install global input hooks once on their dedicated message-loop thread and always remove them during shutdown.
- Never block `WM_MOUSEMOVE` in the low-level mouse hook. A non-zero `WH_MOUSE_LL` result suppresses physical cursor movement. Only consume events that must not reach the target, such as the recognized initial button-down and its matching button-up.
- Never swallow the real modifier key-up. Posting `WM_KEYUP` or `WM_SYSKEYUP` does not update Windows' global keyboard state and can leave Alt or Win logically stuck.
- Coalesce resize requests and apply only the newest rectangle. Do not enqueue one `SetWindowPos` call for every raw mouse event.
- Preserve the original center while both opposite edges have room. When one monitor edge is reached, pin that edge and allow growth toward the remaining free side.
- Respect the target's minimum and maximum tracking sizes, prevent edge inversion, and support negative monitor coordinates.
- Ignore elevated, system, maximized, fullscreen, invisible, disabled, child, cloaked, and non-resizable windows.
- Keep the process Per-Monitor V2 DPI-aware and use physical screen coordinates.
- Do not activate or reorder a target window while resizing it.

## Code conventions

- Keep production code Unicode-only and compile C++ sources with `/utf-8`.
- Use Win32 RAII or explicit paired cleanup for every handle, hook, icon, menu, timer, and GDI object.
- Keep hook callbacks fast. Avoid filesystem, registry, network, allocation-heavy, or unbounded cross-process work in them.
- Use bounded `SendMessageTimeoutW` calls for cross-process window messages.
- Preserve the statically linked C/C++ runtime (`/MT` for Release).
- Keep unrelated user changes intact; do not rewrite or reformat files unnecessarily.

## Localization

The UI supports Russian and English. Any new user-visible string must be added to the localization interface and supplied in both languages. Source files containing localized text must remain UTF-8 and build with `/utf-8`.

Registry names and command-line switches are stable external interfaces; do not localize them.

## Settings and command line

- Settings key: `HKCU\Software\ResizeSymmetrically`.
- Autostart entry: `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`.
- Normal launch opens settings.
- `--startup` starts hidden in the tray.
- `--exit` closes an existing instance and is used by smoke tests.
- `--uninstall-cleanup` removes per-user settings during uninstall.

## Versioning and release artifacts

When changing the application version, keep these values synchronized:

- `installer/ResizeSymmetrically.iss` (`MyAppVersion`);
- `src/app.rc` (`FILEVERSION`, `PRODUCTVERSION`, `FileVersion`, and `ProductVersion`);
- `src/app.manifest` assembly version when appropriate.

Release artifacts are:

- `bin/x64/Release/ResizeSymmetrically.exe`;
- `artifacts/ResizeSymmetrically-Setup-x64.exe`.

Build both artifacts from the same commit before creating a GitHub release.

## Interactive desktop testing

Global hooks and synthesized input affect the user's real desktop. Do not launch GUI automation, move the pointer, or inject keyboard/mouse input unless the user explicitly agrees. Prefer geometry tests and let the user perform manual drag testing when requested.
