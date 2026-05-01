#include "CustomHandlers.h"

namespace {

bool ScrollPageNoCaret(HWND editor, sptr_t direction) {
    sptr_t lines = RunSci(editor, SCI_LINESONSCREEN);
    if (lines < 1) {
        lines = 30;
    }
    RunSci(editor, SCI_LINESCROLL, 0, direction * lines);
    return true;
}

}  // namespace

bool ExecuteScrollPageUpNoCaret(const ActionContext& context) {
    return ScrollPageNoCaret(context.editor, -1);
}

bool ExecuteScrollPageDownNoCaret(const ActionContext& context) {
    return ScrollPageNoCaret(context.editor, 1);
}
