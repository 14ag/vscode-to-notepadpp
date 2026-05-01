# Contributing

Thanks for taking a look at VSCodeKeymapNpp.

Feedback is useful here because shortcut muscle memory is personal, and Notepad++
parity work is full of edge cases. If a shortcut feels wrong, opens the wrong
Notepad++ command, or should map to a better equivalent, please say so.

## Ways to help

You can help by:

1. Reporting shortcut conflicts or behavior gaps.
2. Proposing better Notepad++ or Scintilla equivalents for VS Code commands.
3. Testing on `x86`, `x64`, and `arm64` Notepad++ installs.
4. Sending focused pull requests for mappings, docs, tests, or packaging.

## Before you open an issue

Please include:

1. Notepad++ version and architecture.
2. Plugin version and architecture.
3. Whether strict mode was enabled.
4. The exact VS Code shortcut and expected behavior.
5. What Notepad++ did instead.
6. Steps to reproduce on a clean session if you can.

The bug and feature request templates in `.github/ISSUE_TEMPLATE/` already ask
for most of this.

## Local setup

Prerequisites:

1. CMake 3.20 or newer.
2. Visual Studio 2022 or Build Tools with the Desktop C++ workload.
3. Windows SDK.
4. ARM64 MSVC components if you want to build the ARM64 package locally.

Refresh generated shortcut artifacts after changing the VS Code source snapshot
or the mapping manifest:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\sync-vscode-keybindings.ps1
```

Build and package all release targets:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\package-release.ps1 -Platform all
```

Build one target only:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\package-release.ps1 -Platform x64
```

## Testing expectations

Before sending a PR, run the generator and at least one packaging pass:

1. `powershell -ExecutionPolicy Bypass -File .\scripts\sync-vscode-keybindings.ps1`
2. `powershell -ExecutionPolicy Bypass -File .\scripts\package-release.ps1 -Platform x64`

If your change touches architecture-specific packaging or release metadata, run:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\package-release.ps1 -Platform all
```

For live testing in Notepad++, use an isolated instance where possible:

```powershell
notepad++.exe -multiInst -nosession -settingsDir=<temp-dir>
```

Compare plugin behavior with a control run that disables plugins:

```powershell
notepad++.exe -noPlugin -multiInst -nosession -settingsDir=<temp-dir>
```

## Generated files

These files are generated and should stay in sync:

1. `docs/VSCodeKeybindingCoverage.MD`
2. `src/GeneratedBindings.inc`

If you change `data/keybinding-mappings.json` or the generator script, regenerate
those files in the same change.

## Pull requests

Small, direct PRs are easiest to review.

Please:

1. Explain the user-visible behavior change.
2. Mention any shortcut conflicts that were added, removed, or reserved.
3. Note which test commands you ran.
4. Keep generated output in the same PR when the source data changed.

If you are not sure whether a shortcut should map, reserve, or stay unported,
open an issue first. That kind of discussion is welcome.
