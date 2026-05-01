#include "CustomHandlers.h"

#include "menuCmdID.h"

namespace {

bool DuplicateLineWithCaretTarget(const ActionContext& context, sptr_t targetLineOffset, bool moveDuplicateUp) {
    const bool selectionEmpty = RunSci(context.editor, SCI_GETSELECTIONEMPTY) != 0;
    if (!selectionEmpty) {
        RunNppCommand(context.nppHandle, IDM_EDIT_DUP_LINE);
        if (moveDuplicateUp) {
            RunNppCommand(context.nppHandle, IDM_EDIT_LINE_UP);
        }
        return true;
    }

    const sptr_t currentPos = RunSci(context.editor, SCI_GETCURRENTPOS);
    const sptr_t currentLine = RunSci(context.editor, SCI_LINEFROMPOSITION, static_cast<uptr_t>(currentPos));
    const sptr_t currentColumn = RunSci(context.editor, SCI_GETCOLUMN, static_cast<uptr_t>(currentPos));

    RunNppCommand(context.nppHandle, IDM_EDIT_DUP_LINE);
    if (moveDuplicateUp) {
        RunNppCommand(context.nppHandle, IDM_EDIT_LINE_UP);
    }

    const sptr_t targetLine = currentLine + targetLineOffset;
    const sptr_t targetPos = RunSci(context.editor, SCI_FINDCOLUMN, static_cast<uptr_t>(targetLine), currentColumn);
    if (targetPos >= 0) {
        RunSci(context.editor, SCI_SETSEL, static_cast<uptr_t>(targetPos), targetPos);
        RunSci(context.editor, SCI_SCROLLCARET);
    }

    return true;
}

}  // namespace

bool ExecuteDuplicateLineDown(const ActionContext& context) {
    return DuplicateLineWithCaretTarget(context, 1, false);
}

bool ExecuteDuplicateLineUp(const ActionContext& context) {
    return DuplicateLineWithCaretTarget(context, 0, true);
}

bool ExecuteCutAllowLine(const ActionContext& context) {
    const bool selectionEmpty = RunSci(context.editor, SCI_GETSELECTIONEMPTY) != 0;
    RunSci(context.editor, selectionEmpty ? SCI_CUTALLOWLINE : SCI_CUT);
    return true;
}

bool ExecuteCopyAllowLine(const ActionContext& context) {
    const bool selectionEmpty = RunSci(context.editor, SCI_GETSELECTIONEMPTY) != 0;
    RunSci(context.editor, selectionEmpty ? SCI_COPYALLOWLINE : SCI_COPY);
    return true;
}

bool ExecuteSelectCurrentLine(const ActionContext& context) {
    const sptr_t currentPos = RunSci(context.editor, SCI_GETCURRENTPOS);
    const sptr_t line = RunSci(context.editor, SCI_LINEFROMPOSITION, static_cast<uptr_t>(currentPos));
    const sptr_t lineStart = RunSci(context.editor, SCI_POSITIONFROMLINE, static_cast<uptr_t>(line));
    const sptr_t lineEnd = RunSci(context.editor, SCI_GETLINEENDPOSITION, static_cast<uptr_t>(line));
    RunSci(context.editor, SCI_SETSEL, static_cast<uptr_t>(lineStart), lineEnd);
    return true;
}

bool ExecuteInsertLineBelow(const ActionContext& context) {
    const sptr_t currentPos = RunSci(context.editor, SCI_GETCURRENTPOS);
    const sptr_t line = RunSci(context.editor, SCI_LINEFROMPOSITION, static_cast<uptr_t>(currentPos));
    const sptr_t lineEnd = RunSci(context.editor, SCI_GETLINEENDPOSITION, static_cast<uptr_t>(line));
    RunSci(context.editor, SCI_SETSEL, static_cast<uptr_t>(lineEnd), lineEnd);
    RunSci(context.editor, SCI_NEWLINE);
    return true;
}

bool ExecuteInsertLineAbove(const ActionContext& context) {
    const sptr_t currentPos = RunSci(context.editor, SCI_GETCURRENTPOS);
    const sptr_t line = RunSci(context.editor, SCI_LINEFROMPOSITION, static_cast<uptr_t>(currentPos));
    const sptr_t lineStart = RunSci(context.editor, SCI_POSITIONFROMLINE, static_cast<uptr_t>(line));
    RunSci(context.editor, SCI_SETSEL, static_cast<uptr_t>(lineStart), lineStart);
    RunSci(context.editor, SCI_NEWLINE);
    RunSci(context.editor, SCI_LINEUP);
    RunSci(context.editor, SCI_HOME);
    return true;
}
