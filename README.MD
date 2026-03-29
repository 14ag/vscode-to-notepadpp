# VSCodeKeymapNpp

VSCodeKeymapNpp is a native C++ plugin for Notepad++ that remaps a focused set
of VS Code shortcuts to Notepad++ and Scintilla actions. The goal is simple:
make common editing shortcuts feel closer across both editors without patching
Notepad++ itself.

## What the plugin does

- Intercepts key messages before Notepad++ runs its default accelerators for
  the shortcuts this plugin owns.
- Routes matching shortcuts to built-in Notepad++ commands or direct Scintilla
  messages.
- Implements a few editor actions directly when Notepad++ does not have a close
  built-in match, including insert line above or below, duplicate line up,
  copy or cut whole line with no selection, current-line select, and page
  scroll without moving the caret.
- Treats unsupported VS Code-only shortcuts as reserved no-op bindings in
  strict mode so they do not fall through to unrelated Notepad++ actions.
- Enables multiple selection support on both Scintilla views during plugin
  startup.

## Current behavior

This repository currently builds an x64 plugin and defaults to strict mode.
When strict mode is on, the plugin also handles `Ctrl+K` chords with a 1.5
second timeout. Unknown second keys after a valid `Ctrl+K` prefix are consumed
on purpose so the chord does not leak into a different Notepad++ action.

The plugin registers two menu commands:

- `Plugins > VSCode Keymap NPP > Toggle VSCode Strict Keymap`
- `Plugins > VSCode Keymap NPP > Show Strict Mapping Summary`

## Shortcut coverage

The mapping set is aimed at the shortcuts people tend to reach for first when
switching from VS Code:

- file and tab movement such as `Ctrl+P`, `Ctrl+N`, `Ctrl+W`, `Ctrl+Tab`, and
  `Ctrl+Shift+Tab`
- search and selection shortcuts such as `Ctrl+F`, `Ctrl+H`, `Ctrl+D`,
  `Ctrl+L`, and `Alt+Enter`
- line editing shortcuts such as `Alt+Up`, `Alt+Down`, `Alt+Shift+Up`,
  `Ctrl+Shift+K`, `Ctrl+Enter`, and `Ctrl+Shift+Enter`
- comment, fold, and navigation shortcuts such as `Ctrl+/`, `Ctrl+Shift+\\`,
  `Ctrl+Shift+[`, `Ctrl+Shift+]`, `Ctrl+Home`, and `Ctrl+End`
- `Ctrl+K` chord shortcuts for actions like save all, close all, trim trailing
  whitespace, fold or unfold all, and open the Shortcut Mapper

The list in code is longer than the examples above. The summary command in the
plugin menu is the quickest way to confirm the strict-mode rules inside
Notepad++.

## Build

Prerequisites:

- CMake 3.20 or newer
- Visual Studio or Build Tools with the C++ desktop workload
- A 64-bit Notepad++ installation for runtime testing

Build the DLL:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
```

The build output is `build/Release/VSCodeKeymapNpp.dll`.

## Package a release

This repository includes a packaging script that creates a Plugin Admin-friendly
zip file with the DLL at the root of the archive and the repository README
under `doc/`.

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\package-release.ps1
```

The script reads the version from `CMakeLists.txt` and writes
`dist/VSCodeKeymapNpp-<version>-x64.zip`.

## Install

For a manual install, place the DLL here:

```text
%AppData%\Notepad++\plugins\VSCodeKeymapNpp\VSCodeKeymapNpp.dll
```
or here:
```text
%ProgramFiles%\Notepad++\plugins\VSCodeKeymapNpp\VSCodeKeymapNpp.dll
```

Restart Notepad++ after copying the file. If Windows blocks a DLL downloaded
from the internet, open the file properties first and clear the block before
launching Notepad++.

## Repository layout

- `src/` contains the plugin implementation and the Windows version resource
  template
- `include/` contains the Notepad++ and Scintilla headers used by this build
- `scripts/package-release.ps1` creates the release zip
- `CMakeLists.txt` defines the x64 build and release version

## Limits

This is not a full VS Code emulation layer. The plugin only covers the shortcut
set wired into `src/VSCodeKeymapPlugin.cpp`, and some VS Code-only commands are
intentionally swallowed instead of being mapped to a loose Notepad++ equivalent.
