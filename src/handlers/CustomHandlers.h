#pragma once

#include "ActionHandlers.h"

struct CursorVirtualSpaceOptions {
    bool maintainVirtualSpace = false;
};

CursorVirtualSpaceOptions GetCursorVirtualSpaceOptions();
void SetCursorVirtualSpaceOptions(const CursorVirtualSpaceOptions& options);

bool ExecuteDuplicateLineUp(const ActionContext& context);
bool ExecuteCutAllowLine(const ActionContext& context);
bool ExecuteCopyAllowLine(const ActionContext& context);
bool ExecuteToggleFoldAtCaret(const ActionContext& context);
bool ExecuteToggleStreamComment(const ActionContext& context);
bool ExecuteSelectCurrentLine(const ActionContext& context);
bool ExecuteInsertLineBelow(const ActionContext& context);
bool ExecuteInsertLineAbove(const ActionContext& context);
bool ExecuteInsertCursorBelow(const ActionContext& context);
bool ExecuteInsertCursorAbove(const ActionContext& context);
bool ExecuteCursorColumnSelectDown(const ActionContext& context);
bool ExecuteCursorColumnSelectUp(const ActionContext& context);
bool ExecuteScrollPageUpNoCaret(const ActionContext& context);
bool ExecuteScrollPageDownNoCaret(const ActionContext& context);
