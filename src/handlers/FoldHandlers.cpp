#include "CustomHandlers.h"

bool ExecuteToggleFoldAtCaret(const ActionContext& context) {
    sptr_t line =
        RunSci(context.editor, SCI_LINEFROMPOSITION, static_cast<uptr_t>(RunSci(context.editor, SCI_GETCURRENTPOS)));
    const sptr_t level = RunSci(context.editor, SCI_GETFOLDLEVEL, static_cast<uptr_t>(line));
    if ((level & SC_FOLDLEVELHEADERFLAG) == 0) {
        const sptr_t parent = RunSci(context.editor, SCI_GETFOLDPARENT, static_cast<uptr_t>(line));
        if (parent >= 0) {
            line = parent;
        }
    }
    RunSci(context.editor, SCI_TOGGLEFOLD, static_cast<uptr_t>(line));
    return true;
}
