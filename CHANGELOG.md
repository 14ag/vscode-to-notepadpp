# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project follows semantic versioning.

## [0.2.0] - 2026-04-27

### Added

- Generated shortcut coverage from the stock VS Code Windows defaults page.
- Checked-in source snapshot, mapping manifest, generated runtime tables, and a
  coverage report.
- Release packaging for `x86`, `x64`, and `arm64`.
- `CONTRIBUTING.md` plus GitHub issue templates for shortcut conflicts, bugs,
  and feature requests.

### Changed

- Plugin runtime now consumes generated binding tables instead of a
  hand-maintained shortcut list.
- Conflict handling is stricter for VS Code-owned shortcuts that would
  otherwise trigger the wrong Notepad++ command.
- The plugin summary dialog now reports generated coverage totals.
- Release archives now include `doc/README.MD` and
  `doc/VSCodeKeybindingCoverage.MD`.

### Fixed

- ARM64 packaging now fails fast when the required MSVC components are missing.
- Portable Notepad++ smoke testing was verified against isolated plugin and
  `-noPlugin` control runs.

## [0.1.0] - 2026-03-29

### Added

- Initial public release of the VSCodeKeymapNpp plugin for Notepad++.
