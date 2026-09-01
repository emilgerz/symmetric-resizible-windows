# Resize Symmetrically

[Русский](#русский) · [English](#english)

Небольшая нативная утилита для Windows 11, которая изменяет размер окон симметрично — примерно как Option-resize в macOS.

## Русский

### Возможности

- Симметричное изменение размера окна относительно его центра.
- Работа с границами и углами окна.
- Если одна сторона достигла края монитора, она фиксируется, а окно продолжает расти в свободную сторону.
- Выбор клавиши-модификатора: Alt, Ctrl, Shift или Win.
- Автоматический запуск вместе с Windows.
- Русский и английский интерфейс.
- Работа из системного трея без фонового polling.
- Поддержка мониторов с разным масштабом и отрицательными координатами.

### Использование

1. Запустите `ResizeSymmetrically.exe`.
2. Зажмите выбранную клавишу-модификатор — по умолчанию Alt.
3. Нажмите левую кнопку мыши на изменяемой границе или углу окна и потяните.
4. Отпустите левую кнопку мыши, чтобы завершить изменение размера.

Если отпустить модификатор во время жеста, изменение размера приостановится. Повторное нажатие модификатора возобновит его без скачка окна.

Двойной щелчок по значку в трее открывает настройки. Через контекстное меню значка можно открыть настройки или выйти из программы.

### Установка

Готовые установщик и portable EXE доступны на странице [последнего релиза](https://github.com/emilgerz/symmetric-resizible-windows/releases/latest).

Программа пока не подписана цифровым сертификатом, поэтому Windows SmartScreen может показать предупреждение.

### Ограничения

Утилита намеренно не изменяет размер:

- окон, запущенных с повышенными правами;
- максимизированных и полноэкранных окон;
- системных, скрытых и не поддерживающих изменение размера окон.

Поддерживаются Windows 11 x64 и обычная мышь.

### Сборка

Требования:

- Visual Studio Build Tools с MSVC x64 и Windows 11 SDK;
- Inno Setup 6 — только для сборки установщика.

```powershell
.\build.ps1 -Configuration Release
.\build.ps1 -Configuration Release -Installer
```

Готовый EXE записывается в `bin\x64\Release`, установщик — в `artifacts`.

Параметры запуска:

- обычный запуск открывает настройки;
- `--startup` запускает программу скрыто в трее;
- `--exit` закрывает уже работающий экземпляр и используется в smoke-тестах.

## English

A small native Windows 11 utility that resizes desktop windows symmetrically around their center, similarly to Option-resize on macOS.

### Features

- Symmetric resizing from window edges and corners.
- When one side reaches a monitor edge, it stays pinned while the window continues growing toward the free side.
- Configurable Alt, Ctrl, Shift, or Win modifier.
- Optional per-user startup with Windows.
- Russian and English UI.
- System tray operation without background polling.
- Per-monitor DPI support, including monitors with negative coordinates.

### Usage

1. Run `ResizeSymmetrically.exe`.
2. Hold the configured modifier key (Alt by default).
3. Press the left mouse button on a resizable window edge or corner and drag.
4. Release the left mouse button to finish.

Releasing the modifier pauses the current drag. Pressing it again resumes without a window jump.

Double-click the tray icon to open settings. Its context menu provides Settings and Exit commands.

### Installation

The installer and portable EXE are available from the [latest release](https://github.com/emilgerz/symmetric-resizible-windows/releases/latest).

The application is currently unsigned, so Windows SmartScreen may display a warning.

### Limitations

Elevated, maximized, fullscreen, system, hidden, non-resizable, and otherwise unsupported windows are intentionally ignored. Windows 11 x64 and a standard mouse are supported.

### Build

Requirements:

- Visual Studio Build Tools with MSVC x64 and the Windows 11 SDK;
- Inno Setup 6 only when building the installer.

```powershell
.\build.ps1 -Configuration Release
.\build.ps1 -Configuration Release -Installer
```

The EXE is written to `bin\x64\Release`; the installer is written to `artifacts`.

Command-line behavior:

- a normal launch opens Settings;
- `--startup` starts hidden in the tray;
- `--exit` closes an existing instance and is used by smoke tests.
