#pragma once

#include <cstddef>

#include <windows.h>

#include "Scintilla.h"
#include "menuCmdID.h"

struct KeyStroke {
    bool ctrl;
    bool alt;
    bool shift;
    UINT vk;
};

inline bool operator==(const KeyStroke& lhs, const KeyStroke& rhs) {
    return lhs.ctrl == rhs.ctrl && lhs.alt == rhs.alt && lhs.shift == rhs.shift && lhs.vk == rhs.vk;
}

enum class ActionKind {
    NppCommand,
    SciCommand,
    DuplicateLineUp,
    CutAllowLine,
    CopyAllowLine,
    ToggleFoldAtCaret,
    ToggleStreamComment,
    SelectCurrentLine,
    InsertLineBelow,
    InsertLineAbove,
    InsertCursorBelow,
    InsertCursorAbove,
    CursorColumnSelectDown,
    CursorColumnSelectUp,
    ScrollPageUpNoCaret,
    ScrollPageDownNoCaret,
    ReservedNoOp,
};

struct ShortcutBinding {
    KeyStroke key;
    ActionKind kind;
    int id;
    size_t referenceIndex;
};

struct ChordBinding {
    KeyStroke first;
    KeyStroke second;
    ActionKind kind;
    int id;
    size_t referenceIndex;
};

struct BindingReferenceEntry {
    bool pluginHandled;
    const wchar_t* section;
    const wchar_t* subsection;
    const wchar_t* label;
    const wchar_t* command;
    const wchar_t* winKey;
    const wchar_t* status;
    const wchar_t* handler;
    const wchar_t* target;
    const wchar_t* notes;
};

struct ActionContext {
    HWND nppHandle;
    HWND editor;
};

void RunNppCommand(HWND nppHandle, int commandId);
sptr_t RunSci(HWND editor, UINT message, uptr_t wParam = 0, sptr_t lParam = 0);

bool ExecuteAction(ActionKind kind, int id, const ActionContext& context);
