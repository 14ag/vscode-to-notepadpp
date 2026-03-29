#include <windows.h>

#include <cwchar>
#include <optional>
#include <string>
#include <vector>

#include "PluginInterface.h"
#include "menuCmdID.h"

namespace {
constexpr wchar_t kPluginName[] = L"VSCode Keymap NPP";
constexpr DWORD kChordTimeoutMs = 1500;

NppData g_nppData{};
FuncItem g_funcItems[2]{};
bool g_enabled = true;

WNDPROC g_mainOldProc = nullptr;
WNDPROC g_scintillaOldProc[2] = {nullptr, nullptr};
HHOOK g_messageHook = nullptr;
DWORD g_nppThreadId = 0;

struct KeyStroke {
    bool ctrl;
    bool alt;
    bool shift;
    UINT vk;
};

bool operator==(const KeyStroke& lhs, const KeyStroke& rhs) {
    return lhs.ctrl == rhs.ctrl && lhs.alt == rhs.alt && lhs.shift == rhs.shift && lhs.vk == rhs.vk;
}

enum class ActionKind {
    NppCommand,
    SciCommand,
    DuplicateLineUp,
    CutAllowLine,
    CopyAllowLine,
    ToggleStreamComment,
    SelectCurrentLine,
    InsertLineBelow,
    InsertLineAbove,
    ScrollPageUpNoCaret,
    ScrollPageDownNoCaret,
    ReservedNoOp,
};

struct ShortcutBinding {
    KeyStroke key;
    ActionKind kind;
    int id;
};

struct ChordBinding {
    KeyStroke first;
    KeyStroke second;
    ActionKind kind;
    int id;
};

struct PendingChord {
    KeyStroke first;
    DWORD startedAt;
};

std::optional<PendingChord> g_pendingChord;

HWND ActiveScintilla() {
    int which = 0;
    ::SendMessage(g_nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, reinterpret_cast<LPARAM>(&which));
    return (which == 0) ? g_nppData._scintillaMainHandle : g_nppData._scintillaSecondHandle;
}

int ScintillaIndex(HWND hWnd) {
    if (hWnd == g_nppData._scintillaMainHandle) {
        return 0;
    }
    if (hWnd == g_nppData._scintillaSecondHandle) {
        return 1;
    }
    return -1;
}

bool IsPressed(int vkey) {
    return (::GetKeyState(vkey) & 0x8000) != 0;
}

void RunNppCommand(int commandId) {
    ::SendMessage(g_nppData._nppHandle, NPPM_MENUCOMMAND, 0, static_cast<LPARAM>(commandId));
}

sptr_t RunSci(HWND editor, UINT message, uptr_t wParam = 0, sptr_t lParam = 0) {
    return static_cast<sptr_t>(
        ::SendMessage(editor, message, static_cast<WPARAM>(wParam), static_cast<LPARAM>(lParam)));
}

KeyStroke CurrentKeyStroke(UINT vk) {
    return KeyStroke{IsPressed(VK_CONTROL), IsPressed(VK_MENU), IsPressed(VK_SHIFT), vk};
}

std::string GetLineText(HWND editor, sptr_t line) {
    const sptr_t lengthWithEol = RunSci(editor, SCI_GETLINE, static_cast<uptr_t>(line), 0);
    if (lengthWithEol <= 0) {
        return {};
    }

    std::string buffer(static_cast<size_t>(lengthWithEol), '\0');
    RunSci(editor, SCI_GETLINE, static_cast<uptr_t>(line), reinterpret_cast<sptr_t>(buffer.data()));
    while (!buffer.empty() && (buffer.back() == '\0' || buffer.back() == '\r' || buffer.back() == '\n')) {
        buffer.pop_back();
    }
    return buffer;
}

std::string_view TrimLeftAscii(std::string_view text) {
    size_t i = 0;
    while (i < text.size()) {
        const char ch = text[i];
        if (ch != ' ' && ch != '\t') {
            break;
        }
        ++i;
    }
    return text.substr(i);
}

bool LooksLineCommented(std::string_view trimmed) {
    static constexpr const char* kPrefixes[] = {"//", "#", "--", ";", "'", "REM ", "::"};
    for (const char* prefix : kPrefixes) {
        const size_t prefixLen = std::char_traits<char>::length(prefix);
        if (trimmed.size() >= prefixLen && trimmed.substr(0, prefixLen) == prefix) {
            return true;
        }
    }
    return false;
}

bool ShouldUncommentSelection(HWND editor) {
    const sptr_t selectionStart = RunSci(editor, SCI_GETSELECTIONSTART);
    const sptr_t selectionEnd = RunSci(editor, SCI_GETSELECTIONEND);

    sptr_t startLine = RunSci(editor, SCI_LINEFROMPOSITION, static_cast<uptr_t>(selectionStart));
    sptr_t endLine = RunSci(editor, SCI_LINEFROMPOSITION, static_cast<uptr_t>(selectionEnd));
    if (selectionEnd > selectionStart) {
        const sptr_t endLineStart = RunSci(editor, SCI_POSITIONFROMLINE, static_cast<uptr_t>(endLine));
        if (selectionEnd == endLineStart && endLine > startLine) {
            --endLine;
        }
    }

    bool sawNonEmptyLine = false;
    for (sptr_t line = startLine; line <= endLine; ++line) {
        const std::string text = GetLineText(editor, line);
        const std::string_view trimmed = TrimLeftAscii(text);
        if (trimmed.empty()) {
            continue;
        }
        sawNonEmptyLine = true;
        if (!LooksLineCommented(trimmed)) {
            return false;
        }
    }

    return sawNonEmptyLine;
}

void ConfigureEditor(HWND editor) {
    if (editor == nullptr) {
        return;
    }

    RunSci(editor, SCI_SETMULTIPLESELECTION, 1);
    RunSci(editor, SCI_SETADDITIONALSELECTIONTYPING, 1);
    RunSci(editor, SCI_SETADDITIONALCARETSVISIBLE, 1);
    RunSci(editor, SCI_SETRECTANGULARSELECTIONMODIFIER, SCMOD_ALT);
}

void ConfigureEditors() {
    ConfigureEditor(g_nppData._scintillaMainHandle);
    ConfigureEditor(g_nppData._scintillaSecondHandle);
}

const std::vector<ShortcutBinding> kShortcutBindings = {
    // General
    {{true, false, true, 'P'}, ActionKind::ReservedNoOp, 0},
    {{false, false, false, VK_F1}, ActionKind::ReservedNoOp, 0},
    {{true, false, false, VK_OEM_COMMA}, ActionKind::NppCommand, IDM_SETTING_PREFERENCE},
    {{true, false, false, 'M'}, ActionKind::ReservedNoOp, 0},

    // File management
    {{true, false, false, 'P'}, ActionKind::NppCommand, IDM_FILE_OPEN},
    {{true, false, true, 'N'}, ActionKind::NppCommand, IDM_VIEW_GOTO_NEW_INSTANCE},
    {{true, false, true, 'W'}, ActionKind::NppCommand, IDM_FILE_EXIT},
    {{true, false, false, 'N'}, ActionKind::NppCommand, IDM_FILE_NEW},
    {{true, false, false, 'O'}, ActionKind::NppCommand, IDM_FILE_OPEN},
    {{true, false, false, 'S'}, ActionKind::NppCommand, IDM_FILE_SAVE},
    {{true, false, true, 'S'}, ActionKind::NppCommand, IDM_FILE_SAVEAS},
    {{true, false, false, 'W'}, ActionKind::NppCommand, IDM_FILE_CLOSE},
    {{true, false, false, VK_F4}, ActionKind::NppCommand, IDM_FILE_CLOSE},
    {{true, false, true, 'T'}, ActionKind::NppCommand, IDM_FILE_RESTORELASTCLOSEDFILE},
    {{true, false, false, VK_TAB}, ActionKind::NppCommand, IDM_VIEW_TAB_NEXT},
    {{true, false, true, VK_TAB}, ActionKind::NppCommand, IDM_VIEW_TAB_PREV},
    {{true, false, false, VK_NEXT}, ActionKind::NppCommand, IDM_VIEW_TAB_NEXT},
    {{true, false, false, VK_PRIOR}, ActionKind::NppCommand, IDM_VIEW_TAB_PREV},
    {{true, false, true, VK_PRIOR}, ActionKind::NppCommand, IDM_VIEW_TAB_MOVEBACKWARD},
    {{true, false, true, VK_NEXT}, ActionKind::NppCommand, IDM_VIEW_TAB_MOVEFORWARD},
    {{true, false, false, '1'}, ActionKind::NppCommand, IDM_VIEW_TAB1},
    {{true, false, false, '2'}, ActionKind::NppCommand, IDM_VIEW_TAB2},
    {{true, false, false, '3'}, ActionKind::NppCommand, IDM_VIEW_TAB3},
    {{true, false, false, '4'}, ActionKind::NppCommand, IDM_VIEW_TAB4},
    {{true, false, false, '5'}, ActionKind::NppCommand, IDM_VIEW_TAB5},
    {{true, false, false, '6'}, ActionKind::NppCommand, IDM_VIEW_TAB6},
    {{true, false, false, '7'}, ActionKind::NppCommand, IDM_VIEW_TAB7},
    {{true, false, false, '8'}, ActionKind::NppCommand, IDM_VIEW_TAB8},
    {{true, false, false, '9'}, ActionKind::NppCommand, IDM_VIEW_TAB9},

    // Basic editing
    {{true, false, false, 'X'}, ActionKind::CutAllowLine, 0},
    {{true, false, false, 'C'}, ActionKind::CopyAllowLine, 0},
    {{false, true, false, VK_UP}, ActionKind::NppCommand, IDM_EDIT_LINE_UP},
    {{false, true, false, VK_DOWN}, ActionKind::NppCommand, IDM_EDIT_LINE_DOWN},
    {{false, true, true, VK_DOWN}, ActionKind::NppCommand, IDM_EDIT_DUP_LINE},
    {{false, true, true, VK_UP}, ActionKind::DuplicateLineUp, 0},
    {{true, false, true, 'K'}, ActionKind::SciCommand, SCI_LINEDELETE},
    {{true, false, false, VK_RETURN}, ActionKind::InsertLineBelow, 0},
    {{true, false, true, VK_RETURN}, ActionKind::InsertLineAbove, 0},
    {{true, false, true, VK_OEM_5}, ActionKind::NppCommand, IDM_SEARCH_GOTOMATCHINGBRACE},
    {{true, false, false, VK_OEM_6}, ActionKind::NppCommand, IDM_EDIT_INS_TAB},
    {{true, false, false, VK_OEM_4}, ActionKind::NppCommand, IDM_EDIT_RMV_TAB},
    {{true, false, false, VK_HOME}, ActionKind::SciCommand, SCI_DOCUMENTSTART},
    {{true, false, false, VK_END}, ActionKind::SciCommand, SCI_DOCUMENTEND},
    {{true, false, false, VK_UP}, ActionKind::SciCommand, SCI_LINESCROLLUP},
    {{true, false, false, VK_DOWN}, ActionKind::SciCommand, SCI_LINESCROLLDOWN},
    {{false, true, false, VK_PRIOR}, ActionKind::ScrollPageUpNoCaret, 0},
    {{false, true, false, VK_NEXT}, ActionKind::ScrollPageDownNoCaret, 0},
    {{true, false, true, VK_OEM_4}, ActionKind::NppCommand, IDM_VIEW_FOLD_CURRENT},
    {{true, false, true, VK_OEM_6}, ActionKind::NppCommand, IDM_VIEW_UNFOLD_CURRENT},
    {{true, false, false, VK_OEM_2}, ActionKind::ToggleStreamComment, 0},
    {{true, false, false, VK_DIVIDE}, ActionKind::ToggleStreamComment, 0},
    {{false, true, true, 'A'}, ActionKind::NppCommand, IDM_EDIT_BLOCK_COMMENT},
    {{false, true, false, 'Z'}, ActionKind::NppCommand, IDM_VIEW_WRAP},

    // Navigation and search
    {{true, false, false, 'T'}, ActionKind::NppCommand, IDM_VIEW_SWITCHTO_FUNC_LIST},
    {{true, false, false, 'G'}, ActionKind::NppCommand, IDM_SEARCH_GOTOLINE},
    {{true, false, true, 'O'}, ActionKind::NppCommand, IDM_VIEW_SWITCHTO_FUNC_LIST},
    {{true, false, true, 'M'}, ActionKind::ReservedNoOp, 0},
    {{false, false, false, VK_F8}, ActionKind::ReservedNoOp, 0},
    {{false, false, true, VK_F8}, ActionKind::ReservedNoOp, 0},
    {{false, true, false, VK_LEFT}, ActionKind::ReservedNoOp, 0},
    {{false, true, false, VK_RIGHT}, ActionKind::ReservedNoOp, 0},

    {{true, false, false, 'F'}, ActionKind::NppCommand, IDM_SEARCH_FIND},
    {{true, false, false, 'H'}, ActionKind::NppCommand, IDM_SEARCH_REPLACE},
    {{false, false, false, VK_F3}, ActionKind::NppCommand, IDM_SEARCH_FINDNEXT},
    {{false, false, true, VK_F3}, ActionKind::NppCommand, IDM_SEARCH_FINDPREV},
    {{false, true, false, VK_RETURN}, ActionKind::NppCommand, IDM_EDIT_MULTISELECTALL},
    {{true, false, false, 'D'}, ActionKind::NppCommand, IDM_EDIT_MULTISELECTNEXT},
    {{true, false, false, 'U'}, ActionKind::NppCommand, IDM_EDIT_MULTISELECTUNDO},
    {{true, false, false, 'L'}, ActionKind::SelectCurrentLine, 0},
    {{true, false, true, 'L'}, ActionKind::NppCommand, IDM_EDIT_MULTISELECTALL},
    {{true, false, false, VK_F2}, ActionKind::NppCommand, IDM_EDIT_MULTISELECTALLWHOLEWORD},
    {{false, true, true, VK_RIGHT}, ActionKind::ReservedNoOp, 0},
    {{false, true, true, VK_LEFT}, ActionKind::ReservedNoOp, 0},

    // Keep Ctrl+Alt vertical cursor combos reserved so they don't leak to N++ defaults.
    {{true, true, false, VK_UP}, ActionKind::ReservedNoOp, 0},
    {{true, true, false, VK_DOWN}, ActionKind::ReservedNoOp, 0},
    {{false, true, true, 'I'}, ActionKind::ReservedNoOp, 0},
    {{true, true, true, VK_UP}, ActionKind::ReservedNoOp, 0},
    {{true, true, true, VK_DOWN}, ActionKind::ReservedNoOp, 0},
    {{true, true, true, VK_LEFT}, ActionKind::ReservedNoOp, 0},
    {{true, true, true, VK_RIGHT}, ActionKind::ReservedNoOp, 0},
    {{true, true, true, VK_PRIOR}, ActionKind::ReservedNoOp, 0},
    {{true, true, true, VK_NEXT}, ActionKind::ReservedNoOp, 0},

    // Rich language and symbol tooling
    {{true, false, false, VK_SPACE}, ActionKind::NppCommand, IDM_EDIT_AUTOCOMPLETE},
    {{true, false, false, 'I'}, ActionKind::NppCommand, IDM_EDIT_AUTOCOMPLETE},
    {{true, false, true, VK_SPACE}, ActionKind::NppCommand, IDM_EDIT_FUNCCALLTIP},
    {{false, true, true, 'F'}, ActionKind::ReservedNoOp, 0},
    {{false, false, false, VK_F12}, ActionKind::ReservedNoOp, 0},
    {{false, true, false, VK_F12}, ActionKind::ReservedNoOp, 0},
    {{true, false, false, VK_OEM_PERIOD}, ActionKind::ReservedNoOp, 0},
    {{false, false, true, VK_F12}, ActionKind::ReservedNoOp, 0},
    {{false, false, false, VK_F2}, ActionKind::ReservedNoOp, 0},

    // Editor management and display
    {{true, false, false, VK_OEM_5}, ActionKind::NppCommand, IDM_VIEW_GOTO_ANOTHER_VIEW},
    {{true, false, false, 'B'}, ActionKind::NppCommand, IDM_VIEW_FILEBROWSER},
    {{true, false, true, 'E'}, ActionKind::NppCommand, IDM_VIEW_SWITCHTO_FILEBROWSER},
    {{true, false, true, 'F'}, ActionKind::NppCommand, IDM_SEARCH_FINDINFILES},
    {{true, false, true, 'G'}, ActionKind::ReservedNoOp, 0},
    {{true, false, true, 'D'}, ActionKind::ReservedNoOp, 0},
    {{true, false, true, 'X'}, ActionKind::ReservedNoOp, 0},
    {{true, false, true, 'H'}, ActionKind::NppCommand, IDM_SEARCH_FINDINFILES},
    {{true, false, true, 'J'}, ActionKind::ReservedNoOp, 0},
    {{true, false, true, 'U'}, ActionKind::ReservedNoOp, 0},
    {{true, false, true, 'V'}, ActionKind::ReservedNoOp, 0},
    {{false, false, false, VK_F11}, ActionKind::NppCommand, IDM_VIEW_FULLSCREENTOGGLE},
    {{false, true, true, '0'}, ActionKind::NppCommand, IDM_VIEW_SWITCHTO_OTHER_VIEW},

    // Terminal-ish approximation
    {{true, false, false, VK_OEM_3}, ActionKind::NppCommand, IDM_FILE_OPEN_CMD},
    {{true, false, true, VK_OEM_3}, ActionKind::ReservedNoOp, 0},

    // Reserve conflicting debug function keys from default N++ behavior.
    {{false, false, false, VK_F5}, ActionKind::ReservedNoOp, 0},
    {{false, false, true, VK_F5}, ActionKind::ReservedNoOp, 0},
    {{false, false, false, VK_F9}, ActionKind::ReservedNoOp, 0},
    {{false, false, false, VK_F10}, ActionKind::ReservedNoOp, 0},
};

const std::vector<ChordBinding> kChordBindings = {
    {{true, false, false, 'K'}, {true, false, true, 'S'}, ActionKind::NppCommand, IDM_SETTING_SHORTCUT_MAPPER},
    {{true, false, false, 'K'}, {false, false, false, 'S'}, ActionKind::NppCommand, IDM_FILE_SAVEALL},
    {{true, false, false, 'K'}, {true, false, true, 'W'}, ActionKind::NppCommand, IDM_FILE_CLOSEALL},
    {{true, false, false, 'K'}, {true, false, true, 'C'}, ActionKind::NppCommand, IDM_EDIT_STREAM_COMMENT},
    {{true, false, false, 'K'}, {true, false, true, 'U'}, ActionKind::NppCommand, IDM_EDIT_STREAM_UNCOMMENT},
    {{true, false, false, 'K'}, {true, false, true, '0'}, ActionKind::NppCommand, IDM_VIEW_FOLDALL},
    {{true, false, false, 'K'}, {true, false, true, 'J'}, ActionKind::NppCommand, IDM_VIEW_UNFOLDALL},
    {{true, false, false, 'K'}, {true, false, true, 'D'}, ActionKind::NppCommand, IDM_EDIT_MULTISELECTSSKIP},
    {{true, false, false, 'K'}, {true, false, true, VK_OEM_4}, ActionKind::ReservedNoOp, 0},
    {{true, false, false, 'K'}, {true, false, true, VK_OEM_6}, ActionKind::ReservedNoOp, 0},
    {{true, false, false, 'K'}, {false, false, false, 'P'}, ActionKind::NppCommand, IDM_EDIT_FULLPATHTOCLIP},
    {{true, false, false, 'K'}, {false, false, false, 'R'}, ActionKind::NppCommand, IDM_EDIT_OPENINFOLDER},
    {{true, false, false, 'K'}, {false, false, false, 'O'}, ActionKind::NppCommand, IDM_VIEW_GOTO_NEW_INSTANCE},
    {{true, false, false, 'K'}, {false, false, false, 'Z'}, ActionKind::NppCommand, IDM_VIEW_DISTRACTIONFREE},
    {{true, false, false, 'K'}, {false, false, false, 'V'}, ActionKind::ReservedNoOp, 0},
    {{true, false, false, 'K'}, {false, false, false, 'F'}, ActionKind::ReservedNoOp, 0},
    {{true, false, false, 'K'}, {false, false, false, 'M'}, ActionKind::ReservedNoOp, 0},
    {{true, false, false, 'K'}, {false, false, false, VK_RETURN}, ActionKind::ReservedNoOp, 0},
    {{true, false, false, 'K'}, {false, false, false, VK_LEFT}, ActionKind::ReservedNoOp, 0},
    {{true, false, false, 'K'}, {false, false, false, VK_RIGHT}, ActionKind::ReservedNoOp, 0},
    {{true, false, false, 'K'}, {true, false, false, VK_LEFT}, ActionKind::ReservedNoOp, 0},
    {{true, false, false, 'K'}, {true, false, false, VK_RIGHT}, ActionKind::ReservedNoOp, 0},
    {{true, false, false, 'K'}, {true, false, false, VK_F12}, ActionKind::ReservedNoOp, 0},
    {{true, false, false, 'K'}, {true, false, true, 'X'}, ActionKind::NppCommand, IDM_EDIT_TRIMTRAILING},
};

bool ExecuteAction(ActionKind kind, int id, HWND editor) {
    switch (kind) {
        case ActionKind::NppCommand:
            RunNppCommand(id);
            return true;

        case ActionKind::SciCommand:
            RunSci(editor, static_cast<UINT>(id));
            return true;

        case ActionKind::DuplicateLineUp:
            RunNppCommand(IDM_EDIT_DUP_LINE);
            RunNppCommand(IDM_EDIT_LINE_UP);
            return true;

        case ActionKind::CutAllowLine: {
            const bool selectionEmpty = RunSci(editor, SCI_GETSELECTIONEMPTY) != 0;
            RunSci(editor, selectionEmpty ? SCI_CUTALLOWLINE : SCI_CUT);
            return true;
        }

        case ActionKind::CopyAllowLine: {
            const bool selectionEmpty = RunSci(editor, SCI_GETSELECTIONEMPTY) != 0;
            RunSci(editor, selectionEmpty ? SCI_COPYALLOWLINE : SCI_COPY);
            return true;
        }

        case ActionKind::ToggleStreamComment:
            RunNppCommand(ShouldUncommentSelection(editor) ? IDM_EDIT_STREAM_UNCOMMENT : IDM_EDIT_STREAM_COMMENT);
            return true;

        case ActionKind::SelectCurrentLine: {
            const sptr_t currentPos = RunSci(editor, SCI_GETCURRENTPOS);
            const sptr_t line = RunSci(editor, SCI_LINEFROMPOSITION, static_cast<uptr_t>(currentPos));
            const sptr_t lineStart = RunSci(editor, SCI_POSITIONFROMLINE, static_cast<uptr_t>(line));
            const sptr_t lineEnd = RunSci(editor, SCI_GETLINEENDPOSITION, static_cast<uptr_t>(line));
            RunSci(editor, SCI_SETSEL, static_cast<uptr_t>(lineStart), lineEnd);
            return true;
        }

        case ActionKind::InsertLineBelow: {
            const sptr_t currentPos = RunSci(editor, SCI_GETCURRENTPOS);
            const sptr_t line = RunSci(editor, SCI_LINEFROMPOSITION, static_cast<uptr_t>(currentPos));
            const sptr_t lineEnd = RunSci(editor, SCI_GETLINEENDPOSITION, static_cast<uptr_t>(line));
            RunSci(editor, SCI_SETSEL, static_cast<uptr_t>(lineEnd), lineEnd);
            RunSci(editor, SCI_NEWLINE);
            return true;
        }

        case ActionKind::InsertLineAbove: {
            const sptr_t currentPos = RunSci(editor, SCI_GETCURRENTPOS);
            const sptr_t line = RunSci(editor, SCI_LINEFROMPOSITION, static_cast<uptr_t>(currentPos));
            const sptr_t lineStart = RunSci(editor, SCI_POSITIONFROMLINE, static_cast<uptr_t>(line));
            RunSci(editor, SCI_SETSEL, static_cast<uptr_t>(lineStart), lineStart);
            RunSci(editor, SCI_NEWLINE);
            RunSci(editor, SCI_LINEUP);
            RunSci(editor, SCI_HOME);
            return true;
        }

        case ActionKind::ScrollPageUpNoCaret: {
            sptr_t lines = RunSci(editor, SCI_LINESONSCREEN);
            if (lines < 1) {
                lines = 30;
            }
            RunSci(editor, SCI_LINESCROLL, 0, -lines);
            return true;
        }

        case ActionKind::ScrollPageDownNoCaret: {
            sptr_t lines = RunSci(editor, SCI_LINESONSCREEN);
            if (lines < 1) {
                lines = 30;
            }
            RunSci(editor, SCI_LINESCROLL, 0, lines);
            return true;
        }

        case ActionKind::ReservedNoOp:
            return true;
    }

    return false;
}

bool TryHandleChord(const KeyStroke& currentKey, HWND editor) {
    if (g_pendingChord.has_value()) {
        const DWORD age = ::GetTickCount() - g_pendingChord->startedAt;
        if (age <= kChordTimeoutMs) {
            for (const auto& chord : kChordBindings) {
                if (chord.first == g_pendingChord->first && chord.second == currentKey) {
                    g_pendingChord.reset();
                    return ExecuteAction(chord.kind, chord.id, editor);
                }
            }

            // Strict mode: consume unknown second key after a valid chord prefix.
            g_pendingChord.reset();
            return true;
        }

        g_pendingChord.reset();
    }

    for (const auto& chord : kChordBindings) {
        if (chord.first == currentKey) {
            g_pendingChord = PendingChord{currentKey, ::GetTickCount()};
            return true;
        }
    }

    return false;
}

bool TryHandleShortcut(HWND editor, WPARAM wParam) {
    if (!g_enabled || editor == nullptr) {
        return false;
    }

    const UINT vk = static_cast<UINT>(wParam);
    if (vk == VK_CONTROL || vk == VK_MENU || vk == VK_SHIFT) {
        return false;
    }

    const KeyStroke currentKey = CurrentKeyStroke(vk);

    if (TryHandleChord(currentKey, editor)) {
        return true;
    }

    for (const auto& binding : kShortcutBindings) {
        if (binding.key == currentKey) {
            return ExecuteAction(binding.kind, binding.id, editor);
        }
    }

    if (currentKey.ctrl && !currentKey.alt && !currentKey.shift && (vk == VK_OEM_PLUS || vk == VK_ADD)) {
        RunNppCommand(IDM_VIEW_ZOOMIN);
        return true;
    }

    if (currentKey.ctrl && !currentKey.alt && !currentKey.shift && (vk == VK_OEM_MINUS || vk == VK_SUBTRACT)) {
        RunNppCommand(IDM_VIEW_ZOOMOUT);
        return true;
    }

    if (currentKey.ctrl && !currentKey.alt && !currentKey.shift && vk == '0') {
        RunNppCommand(IDM_VIEW_ZOOMRESTORE);
        return true;
    }

    return false;
}

LRESULT CALLBACK MessageHookProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code >= 0 && wParam == PM_REMOVE) {
        MSG* msg = reinterpret_cast<MSG*>(lParam);
        if (msg != nullptr && (msg->message == WM_KEYDOWN || msg->message == WM_SYSKEYDOWN)) {
            if (TryHandleShortcut(ActiveScintilla(), msg->wParam)) {
                msg->message = WM_NULL;
                msg->wParam = 0;
                msg->lParam = 0;
                return 1;
            }
        }
    }

    return ::CallNextHookEx(g_messageHook, code, wParam, lParam);
}

LRESULT CALLBACK ScintillaProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if ((msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) && TryHandleShortcut(hWnd, wParam)) {
        return 0;
    }

    const int idx = ScintillaIndex(hWnd);
    if (idx >= 0 && g_scintillaOldProc[idx] != nullptr) {
        return ::CallWindowProc(g_scintillaOldProc[idx], hWnd, msg, wParam, lParam);
    }

    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK MainProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if ((msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) && TryHandleShortcut(ActiveScintilla(), wParam)) {
        return 0;
    }

    if (g_mainOldProc != nullptr) {
        return ::CallWindowProc(g_mainOldProc, hWnd, msg, wParam, lParam);
    }

    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

void HookWindows() {
    if (g_nppData._nppHandle != nullptr && g_mainOldProc == nullptr) {
        g_mainOldProc = reinterpret_cast<WNDPROC>(
            ::SetWindowLongPtr(g_nppData._nppHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(MainProc)));
    }

    if (g_nppData._scintillaMainHandle != nullptr && g_scintillaOldProc[0] == nullptr) {
        g_scintillaOldProc[0] = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(
            g_nppData._scintillaMainHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(ScintillaProc)));
    }

    if (g_nppData._scintillaSecondHandle != nullptr && g_scintillaOldProc[1] == nullptr) {
        g_scintillaOldProc[1] = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(
            g_nppData._scintillaSecondHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(ScintillaProc)));
    }
}

void UnhookWindows() {
    if (g_nppData._nppHandle != nullptr && g_mainOldProc != nullptr) {
        ::SetWindowLongPtr(g_nppData._nppHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_mainOldProc));
        g_mainOldProc = nullptr;
    }

    if (g_nppData._scintillaMainHandle != nullptr && g_scintillaOldProc[0] != nullptr) {
        ::SetWindowLongPtr(g_nppData._scintillaMainHandle, GWLP_WNDPROC,
                           reinterpret_cast<LONG_PTR>(g_scintillaOldProc[0]));
        g_scintillaOldProc[0] = nullptr;
    }

    if (g_nppData._scintillaSecondHandle != nullptr && g_scintillaOldProc[1] != nullptr) {
        ::SetWindowLongPtr(g_nppData._scintillaSecondHandle, GWLP_WNDPROC,
                           reinterpret_cast<LONG_PTR>(g_scintillaOldProc[1]));
        g_scintillaOldProc[1] = nullptr;
    }
}

void InstallMessageHook() {
    if (g_messageHook != nullptr || g_nppData._nppHandle == nullptr) {
        return;
    }

    g_nppThreadId = ::GetWindowThreadProcessId(g_nppData._nppHandle, nullptr);
    if (g_nppThreadId == 0) {
        return;
    }

    g_messageHook = ::SetWindowsHookEx(WH_GETMESSAGE, MessageHookProc, nullptr, g_nppThreadId);
}

void RemoveMessageHook() {
    if (g_messageHook != nullptr) {
        ::UnhookWindowsHookEx(g_messageHook);
        g_messageHook = nullptr;
    }

    g_nppThreadId = 0;
    g_pendingChord.reset();
}

void ToggleKeymap() {
    g_enabled = !g_enabled;
    g_pendingChord.reset();

    const wchar_t* status = g_enabled ? L"enabled" : L"disabled";
    std::wstring message = L"VSCode strict keymap is now ";
    message += status;
    message += L".";
    ::MessageBox(g_nppData._nppHandle, message.c_str(), kPluginName, MB_OK | MB_ICONINFORMATION);
}

void ShowBindingsSummary() {
    const wchar_t* summary =
        L"VSCode Keymap NPP strict mode\n\n"
        L"- Enforces near 1:1 mappings where Notepad++ equivalents exist\n"
        L"- Implements missing line operations (move/duplicate/insert/delete)\n"
        L"- Uses a pre-accelerator hook to block conflicting default N++ shortcuts\n"
        L"- Unsupported VSCode-only shortcuts are reserved as no-op\n"
        L"  so they do not trigger non-VSCode Notepad++ actions.";
    ::MessageBox(g_nppData._nppHandle, summary, kPluginName, MB_OK | MB_ICONINFORMATION);
}

void RegisterPluginCommands() {
    std::wmemset(g_funcItems[0]._itemName, 0, menuItemSize);
    std::wmemset(g_funcItems[1]._itemName, 0, menuItemSize);

    ::wcsncpy_s(g_funcItems[0]._itemName, menuItemSize, L"Toggle VSCode Strict Keymap", _TRUNCATE);
    g_funcItems[0]._pFunc = ToggleKeymap;

    ::wcsncpy_s(g_funcItems[1]._itemName, menuItemSize, L"Show Strict Mapping Summary", _TRUNCATE);
    g_funcItems[1]._pFunc = ShowBindingsSummary;
}

}  // namespace

extern "C" __declspec(dllexport) void setInfo(NppData notepadPlusData) {
    g_nppData = notepadPlusData;
    RegisterPluginCommands();
    HookWindows();
    ConfigureEditors();
    InstallMessageHook();
}

extern "C" __declspec(dllexport) const wchar_t* getName() {
    return kPluginName;
}

extern "C" __declspec(dllexport) FuncItem* getFuncsArray(int* nbF) {
    *nbF = 2;
    return g_funcItems;
}

extern "C" __declspec(dllexport) void beNotified(SCNotification*) {
    // Not used for this implementation.
}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT, WPARAM, LPARAM) {
    return TRUE;
}

extern "C" __declspec(dllexport) BOOL isUnicode() {
    return TRUE;
}

extern "C" __declspec(dllexport) void cleanUp() {
    RemoveMessageHook();
    UnhookWindows();
}
