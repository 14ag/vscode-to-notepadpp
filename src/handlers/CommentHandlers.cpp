#include "CustomHandlers.h"

#include <string>
#include <string_view>

#include "menuCmdID.h"

namespace {

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

}  // namespace

bool ExecuteToggleStreamComment(const ActionContext& context) {
    RunNppCommand(context.nppHandle,
                  ShouldUncommentSelection(context.editor) ? IDM_EDIT_STREAM_UNCOMMENT : IDM_EDIT_STREAM_COMMENT);
    return true;
}
