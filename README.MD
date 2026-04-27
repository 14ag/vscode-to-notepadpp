# VSCodeKeymapNpp

VSCodeKeymapNpp is a native C++ plugin for Notepad++ that ports the stock
Windows VS Code shortcut set as far as Notepad++ can reasonably follow it.
Where there is a clean Notepad++ or Scintilla equivalent, the plugin maps it.
Where there is not, the shortcut is either reserved as a strict no-op or
documented as intentionally unported.

## What changed

- Source of truth is now the current VS Code Windows default shortcuts page:
  `https://code.visualstudio.com/docs/reference/default-keybindings`
- Coverage is generated, not hand-curated
- Runtime bindings come from generated tables in `src/GeneratedBindings.inc`
- Release flow now supports `x86`, `x64`, and `arm64`

The plugin still registers two menu commands:

- `Plugins > VSCode Keymap NPP > Toggle VSCode Strict Keymap`
- `Plugins > VSCode Keymap NPP > Show VSCode Coverage Summary`

## Coverage model

Each Windows-default VS Code binding is tracked in
[docs/VSCodeKeybindingCoverage.MD](docs/VSCodeKeybindingCoverage.MD).

Bindings land in one of three states:

- `mapped`: handled by Notepad++, Scintilla, or plugin custom logic
- `reserved-noop`: swallowed so unrelated Notepad++ behavior does not fire
- `documented-unported`: listed, but intentionally not emulated

Current generated totals live in the coverage report and are also shown in the
plugin summary dialog.

## Repository layout

- `src/VSCodeKeymapPlugin.cpp`: plugin runtime and custom editor actions
- `src/GeneratedBindings.inc`: generated runtime binding tables
- `data/keybinding-mappings.json`: mapping manifest for portable shortcuts
- `data/vscode-default-keybindings.windows.json`: generated normalized source snapshot
- `docs/VSCodeKeybindingCoverage.MD`: generated coverage report
- `scripts/sync-vscode-keybindings.ps1`: refresh source snapshot, report, and runtime tables
- `scripts/package-release.ps1`: build and package release zips

## Refresh generated shortcut data

Run this whenever the VS Code defaults page changes or when you update the
mapping manifest:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\sync-vscode-keybindings.ps1
```

That script will:

- fetch the current VS Code default keybindings page
- regenerate the normalized Windows shortcut snapshot
- validate manifest coverage and runtime collisions
- regenerate the runtime include file
- regenerate the Markdown coverage report

## Build

Prerequisites:

- CMake 3.20 or newer
- Visual Studio 2022 or Build Tools with Desktop C++ workload
- Windows SDK
- For ARM64 builds: Visual Studio ARM64 C++ toolchain/components

Manual build examples:

```powershell
cmake -S . -B build-x86 -A Win32
cmake --build build-x86 --config Release

cmake -S . -B build-x64 -A x64
cmake --build build-x64 --config Release

cmake -S . -B build-arm64 -A ARM64
cmake --build build-arm64 --config Release
```

Outputs:

- `build-x86\Release\VSCodeKeymapNpp.dll`
- `build-x64\Release\VSCodeKeymapNpp.dll`
- `build-arm64\Release\VSCodeKeymapNpp.dll`

## Package release zips

Build and package one architecture:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\package-release.ps1 -Platform x64
```

Build and package all architectures:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\package-release.ps1 -Platform all
```

Skip build and package from existing outputs:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\package-release.ps1 -Platform x86 -SkipBuild
```

Artifacts:

- `dist/VSCodeKeymapNpp-<version>-x86.zip`
- `dist/VSCodeKeymapNpp-<version>-x64.zip`
- `dist/VSCodeKeymapNpp-<version>-arm64.zip`

Each zip is Plugin Admin-friendly:

- `VSCodeKeymapNpp.dll` at archive root
- `doc/README.MD`
- `doc/VSCodeKeybindingCoverage.MD`

## Install

Manual install path:

```text
%AppData%\Notepad++\plugins\VSCodeKeymapNpp\VSCodeKeymapNpp.dll
```

Or under a machine-wide Notepad++ install:

```text
%ProgramFiles%\Notepad++\plugins\VSCodeKeymapNpp\VSCodeKeymapNpp.dll
%ProgramFiles(x86)%\Notepad++\plugins\VSCodeKeymapNpp\VSCodeKeymapNpp.dll
```

Use the DLL whose architecture matches the installed Notepad++ build exactly.
Restart Notepad++ after copying the plugin.

## Feedback and contributions

If a shortcut still behaves differently from VS Code, opens the wrong
Notepad++ command, or should be mapped to a better equivalent, please open an
issue. Shortcut parity is easiest to improve with concrete reproduction steps
from real editor use.

- Contribution guide: [CONTRIBUTING.md](CONTRIBUTING.md)
- Change history: [CHANGELOG.md](CHANGELOG.md)

## Limits

This is still not a full VS Code emulation layer. Workbench-only features like
debug panes, Problems, hover, format providers, theme picker, and extension UI
remain outside clean Notepad++ parity, so they stay reserved or documented as
unported instead of being mapped loosely.
