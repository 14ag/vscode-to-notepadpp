#include "ActionHandlers.h"

#include "CustomHandlers.h"
#include "Notepad_plus_msgs.h"

void RunNppCommand(HWND nppHandle, int commandId) {
    ::SendMessage(nppHandle, NPPM_MENUCOMMAND, 0, static_cast<LPARAM>(commandId));
}

sptr_t RunSci(HWND editor, UINT message, uptr_t wParam, sptr_t lParam) {
    return static_cast<sptr_t>(
        ::SendMessage(editor, message, static_cast<WPARAM>(wParam), static_cast<LPARAM>(lParam)));
}

bool ExecuteAction(ActionKind kind, int id, const ActionContext& context) {
    switch (kind) {
        case ActionKind::NppCommand:
            RunNppCommand(context.nppHandle, id);
            return true;

        case ActionKind::SciCommand:
            RunSci(context.editor, static_cast<UINT>(id));
            return true;

        case ActionKind::DuplicateLineDown:
            return ExecuteDuplicateLineDown(context);

        case ActionKind::DuplicateLineUp:
            return ExecuteDuplicateLineUp(context);

        case ActionKind::CutAllowLine:
            return ExecuteCutAllowLine(context);

        case ActionKind::CopyAllowLine:
            return ExecuteCopyAllowLine(context);

        case ActionKind::ToggleFoldAtCaret:
            return ExecuteToggleFoldAtCaret(context);

        case ActionKind::ToggleStreamComment:
            return ExecuteToggleStreamComment(context);

        case ActionKind::SelectCurrentLine:
            return ExecuteSelectCurrentLine(context);

        case ActionKind::InsertLineBelow:
            return ExecuteInsertLineBelow(context);

        case ActionKind::InsertLineAbove:
            return ExecuteInsertLineAbove(context);

        case ActionKind::InsertCursorBelow:
            return ExecuteInsertCursorBelow(context);

        case ActionKind::InsertCursorAbove:
            return ExecuteInsertCursorAbove(context);

        case ActionKind::CursorColumnSelectDown:
            return ExecuteCursorColumnSelectDown(context);

        case ActionKind::CursorColumnSelectUp:
            return ExecuteCursorColumnSelectUp(context);

        case ActionKind::ScrollPageUpNoCaret:
            return ExecuteScrollPageUpNoCaret(context);

        case ActionKind::ScrollPageDownNoCaret:
            return ExecuteScrollPageDownNoCaret(context);

        case ActionKind::ReservedNoOp:
            return true;
    }

    return false;
}
