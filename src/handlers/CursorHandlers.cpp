#include "CustomHandlers.h"

#include <optional>
#include <vector>

namespace {

struct SelectionEndpoint {
    sptr_t position;
    sptr_t virtualSpace;
};

struct SelectionState {
    SelectionEndpoint caret;
    SelectionEndpoint anchor;
};

struct SelectionColumns {
    sptr_t caret;
    sptr_t anchor;
};

struct VerticalSelectionState {
    HWND editor;
    SelectionState selection;
    SelectionColumns columns;
    bool active;
};

struct ColumnSelectionState {
    VerticalSelectionState anchor;
    sptr_t extent;
};

ColumnSelectionState g_columnSelection{};
VerticalSelectionState g_insertCursor{};
CursorVirtualSpaceOptions g_cursorVirtualSpaceOptions{};

class ScopedCaretBlinkPause {
public:
    explicit ScopedCaretBlinkPause(HWND editor)
        : editor_(editor),
          caretPeriod_(RunSci(editor, SCI_GETCARETPERIOD)),
          additionalCaretsBlink_(RunSci(editor, SCI_GETADDITIONALCARETSBLINK)) {
        RunSci(editor_, SCI_SETCARETPERIOD, 0);
        RunSci(editor_, SCI_SETADDITIONALCARETSBLINK, 0);
    }

    ~ScopedCaretBlinkPause() {
        RunSci(editor_, SCI_SETCARETPERIOD, static_cast<uptr_t>(caretPeriod_));
        RunSci(editor_, SCI_SETADDITIONALCARETSBLINK, static_cast<uptr_t>(additionalCaretsBlink_));
    }

    ScopedCaretBlinkPause(const ScopedCaretBlinkPause&) = delete;
    ScopedCaretBlinkPause& operator=(const ScopedCaretBlinkPause&) = delete;

private:
    HWND editor_;
    sptr_t caretPeriod_;
    sptr_t additionalCaretsBlink_;
};

SelectionState GetSelectionState(HWND editor, sptr_t selectionIndex) {
    return SelectionState{
        {RunSci(editor, SCI_GETSELECTIONNCARET, static_cast<uptr_t>(selectionIndex)),
         RunSci(editor, SCI_GETSELECTIONNCARETVIRTUALSPACE, static_cast<uptr_t>(selectionIndex))},
        {RunSci(editor, SCI_GETSELECTIONNANCHOR, static_cast<uptr_t>(selectionIndex)),
         RunSci(editor, SCI_GETSELECTIONNANCHORVIRTUALSPACE, static_cast<uptr_t>(selectionIndex))},
    };
}

bool SameSelectionState(const SelectionState& lhs, const SelectionState& rhs) {
    return lhs.caret.position == rhs.caret.position && lhs.caret.virtualSpace == rhs.caret.virtualSpace &&
           lhs.anchor.position == rhs.anchor.position && lhs.anchor.virtualSpace == rhs.anchor.virtualSpace;
}

sptr_t EndpointColumn(HWND editor, const SelectionEndpoint& endpoint) {
    return RunSci(editor, SCI_GETCOLUMN, static_cast<uptr_t>(endpoint.position)) + endpoint.virtualSpace;
}

sptr_t EndpointLine(HWND editor, const SelectionEndpoint& endpoint) {
    return RunSci(editor, SCI_LINEFROMPOSITION, static_cast<uptr_t>(endpoint.position));
}

SelectionColumns GetSelectionColumns(HWND editor, const SelectionState& selection) {
    return SelectionColumns{EndpointColumn(editor, selection.caret), EndpointColumn(editor, selection.anchor)};
}

std::optional<SelectionState> GetMainSelection(HWND editor) {
    const sptr_t selectionCount = RunSci(editor, SCI_GETSELECTIONS);
    if (selectionCount <= 0) {
        return std::nullopt;
    }

    const sptr_t mainSelection = RunSci(editor, SCI_GETMAINSELECTION);
    if (mainSelection < 0 || mainSelection >= selectionCount) {
        return std::nullopt;
    }

    return GetSelectionState(editor, mainSelection);
}

std::optional<SelectionEndpoint> MoveEndpoint(HWND editor,
                                              const SelectionEndpoint& endpoint,
                                              sptr_t lineDelta,
                                              sptr_t desiredColumn,
                                              bool maintainVirtualSpace) {
    const sptr_t currentLine = RunSci(editor, SCI_LINEFROMPOSITION, static_cast<uptr_t>(endpoint.position));
    const sptr_t targetLine = currentLine + lineDelta;
    const sptr_t lineCount = RunSci(editor, SCI_GETLINECOUNT);
    if (targetLine < 0 || targetLine >= lineCount) {
        return std::nullopt;
    }

    const sptr_t targetPos =
        RunSci(editor, SCI_FINDCOLUMN, static_cast<uptr_t>(targetLine), desiredColumn);
    if (targetPos < 0) {
        return std::nullopt;
    }

    const sptr_t actualColumn = RunSci(editor, SCI_GETCOLUMN, static_cast<uptr_t>(targetPos));
    const sptr_t targetVirtualSpace =
        (maintainVirtualSpace && desiredColumn > actualColumn) ? (desiredColumn - actualColumn) : 0;
    return SelectionEndpoint{targetPos, targetVirtualSpace};
}

std::optional<SelectionState> MoveSelection(HWND editor,
                                            const SelectionState& selection,
                                            sptr_t lineDelta,
                                            const SelectionColumns& columns,
                                            bool maintainVirtualSpace) {
    const auto movedCaret = MoveEndpoint(editor, selection.caret, lineDelta, columns.caret, maintainVirtualSpace);
    if (!movedCaret.has_value()) {
        return std::nullopt;
    }

    const auto movedAnchor = MoveEndpoint(editor, selection.anchor, lineDelta, columns.anchor, maintainVirtualSpace);
    if (!movedAnchor.has_value()) {
        return std::nullopt;
    }

    return SelectionState{*movedCaret, *movedAnchor};
}

sptr_t FindSelectionState(HWND editor, const SelectionState& selection) {
    const sptr_t selectionCount = RunSci(editor, SCI_GETSELECTIONS);
    for (sptr_t i = 0; i < selectionCount; ++i) {
        if (SameSelectionState(GetSelectionState(editor, i), selection)) {
            return i;
        }
    }
    return -1;
}

sptr_t FindSelectionOnCaretLine(HWND editor, const SelectionState& selection) {
    const sptr_t targetLine = EndpointLine(editor, selection.caret);
    const sptr_t selectionCount = RunSci(editor, SCI_GETSELECTIONS);
    for (sptr_t i = 0; i < selectionCount; ++i) {
        if (EndpointLine(editor, GetSelectionState(editor, i).caret) == targetLine) {
            return i;
        }
    }
    return -1;
}

void SetSelectionState(HWND editor, sptr_t selectionIndex, const SelectionState& selection) {
    RunSci(editor, SCI_SETSELECTIONNCARET, static_cast<uptr_t>(selectionIndex), selection.caret.position);
    RunSci(editor, SCI_SETSELECTIONNANCHOR, static_cast<uptr_t>(selectionIndex), selection.anchor.position);
    RunSci(editor, SCI_SETSELECTIONNCARETVIRTUALSPACE, static_cast<uptr_t>(selectionIndex),
           selection.caret.virtualSpace);
    RunSci(editor, SCI_SETSELECTIONNANCHORVIRTUALSPACE, static_cast<uptr_t>(selectionIndex),
           selection.anchor.virtualSpace);
}

sptr_t AbsExtent(sptr_t value) {
    return value < 0 ? -value : value;
}

std::optional<std::vector<SelectionState>> BuildColumnSelections(HWND editor,
                                                                 const VerticalSelectionState& anchor,
                                                                 sptr_t extent) {
    const sptr_t firstDelta = extent < 0 ? extent : 0;
    const sptr_t lastDelta = extent > 0 ? extent : 0;
    std::vector<SelectionState> selections;
    selections.reserve(static_cast<size_t>(lastDelta - firstDelta + 1));

    for (sptr_t delta = firstDelta; delta <= lastDelta; ++delta) {
        const auto movedSelection =
            MoveSelection(editor,
                          anchor.selection,
                          delta,
                          anchor.columns,
                          g_cursorVirtualSpaceOptions.maintainVirtualSpace);
        if (!movedSelection.has_value()) {
            return std::nullopt;
        }
        selections.push_back(*movedSelection);
    }

    return selections;
}

bool ApplySelectionSet(HWND editor, const std::vector<SelectionState>& selections, sptr_t mainSelectionIndex) {
    if (selections.empty()) {
        return false;
    }

    RunSci(editor, SCI_SETSELECTION, static_cast<uptr_t>(selections[0].caret.position),
           selections[0].anchor.position);
    SetSelectionState(editor, 0, selections[0]);

    for (size_t i = 1; i < selections.size(); ++i) {
        RunSci(editor, SCI_ADDSELECTION, static_cast<uptr_t>(selections[i].caret.position),
               selections[i].anchor.position);
        SetSelectionState(editor, static_cast<sptr_t>(i), selections[i]);
    }

    RunSci(editor, SCI_SETMAINSELECTION, static_cast<uptr_t>(mainSelectionIndex));
    RunSci(editor, SCI_SCROLLCARET);
    return true;
}

bool ApplyColumnSelection(HWND editor, const VerticalSelectionState& anchor, sptr_t extent) {
    const auto selections = BuildColumnSelections(editor, anchor, extent);
    if (!selections.has_value()) {
        return false;
    }

    const sptr_t firstDelta = extent < 0 ? extent : 0;
    const sptr_t mainSelectionIndex = extent - firstDelta;
    return ApplySelectionSet(editor, *selections, mainSelectionIndex);
}

void ResetColumnSelection() {
    g_columnSelection = ColumnSelectionState{};
}

void ResetInsertCursorSelection() {
    g_insertCursor = VerticalSelectionState{};
}

bool IsColumnSelectionCurrent(HWND editor) {
    if (!g_columnSelection.anchor.active || g_columnSelection.anchor.editor != editor) {
        return false;
    }

    const sptr_t expectedCount = AbsExtent(g_columnSelection.extent) + 1;
    if (RunSci(editor, SCI_GETSELECTIONS) != expectedCount) {
        return false;
    }

    const auto selections = BuildColumnSelections(editor, g_columnSelection.anchor, g_columnSelection.extent);
    if (!selections.has_value()) {
        return false;
    }

    for (const auto& selection : *selections) {
        if (FindSelectionState(editor, selection) < 0) {
            return false;
        }
    }

    return true;
}

bool StartColumnSelection(HWND editor) {
    const auto mainSelection = GetMainSelection(editor);
    if (!mainSelection.has_value()) {
        return false;
    }

    g_columnSelection = ColumnSelectionState{
        VerticalSelectionState{editor, *mainSelection, GetSelectionColumns(editor, *mainSelection), true}, 0};
    return true;
}

bool UpdateColumnSelection(HWND editor, sptr_t direction) {
    ScopedCaretBlinkPause caretBlinkPause(editor);

    if (!IsColumnSelectionCurrent(editor) && !StartColumnSelection(editor)) {
        return true;
    }

    const sptr_t newExtent = g_columnSelection.extent + direction;
    if (!ApplyColumnSelection(editor, g_columnSelection.anchor, newExtent)) {
        return true;
    }

    g_columnSelection.extent = newExtent;
    return true;
}

bool InsertCursorOnAdjacentLine(HWND editor, sptr_t lineDelta) {
    ScopedCaretBlinkPause caretBlinkPause(editor);

    const auto currentSelection = GetMainSelection(editor);
    if (!currentSelection.has_value()) {
        return true;
    }

    if (!g_insertCursor.active || g_insertCursor.editor != editor ||
        !SameSelectionState(g_insertCursor.selection, *currentSelection)) {
        g_insertCursor = VerticalSelectionState{
            editor, *currentSelection, GetSelectionColumns(editor, *currentSelection), true};
    }

    const auto movedSelection = MoveSelection(
        editor,
        *currentSelection,
        lineDelta,
        g_insertCursor.columns,
        g_cursorVirtualSpaceOptions.maintainVirtualSpace);
    if (!movedSelection.has_value()) {
        return true;
    }

    const sptr_t existingSelection = FindSelectionOnCaretLine(editor, *movedSelection);
    if (existingSelection >= 0) {
        RunSci(editor, SCI_SETMAINSELECTION, static_cast<uptr_t>(existingSelection));
        RunSci(editor, SCI_SCROLLCARET);
        g_insertCursor.selection = GetSelectionState(editor, existingSelection);
        return true;
    }

    RunSci(editor, SCI_ADDSELECTION, static_cast<uptr_t>(movedSelection->caret.position),
           movedSelection->anchor.position);
    const sptr_t newSelectionIndex = RunSci(editor, SCI_GETMAINSELECTION);
    if (newSelectionIndex < 0) {
        return true;
    }
    SetSelectionState(editor, newSelectionIndex, *movedSelection);
    RunSci(editor, SCI_SETMAINSELECTION, static_cast<uptr_t>(newSelectionIndex));
    RunSci(editor, SCI_SCROLLCARET);
    g_insertCursor.selection = *movedSelection;
    return true;
}

}  // namespace

CursorVirtualSpaceOptions GetCursorVirtualSpaceOptions() {
    return g_cursorVirtualSpaceOptions;
}

void SetCursorVirtualSpaceOptions(const CursorVirtualSpaceOptions& options) {
    g_cursorVirtualSpaceOptions = options;
    ResetColumnSelection();
    ResetInsertCursorSelection();
}

bool ExecuteInsertCursorBelow(const ActionContext& context) {
    ResetColumnSelection();
    return InsertCursorOnAdjacentLine(context.editor, 1);
}

bool ExecuteInsertCursorAbove(const ActionContext& context) {
    ResetColumnSelection();
    return InsertCursorOnAdjacentLine(context.editor, -1);
}

bool ExecuteCursorColumnSelectDown(const ActionContext& context) {
    ResetInsertCursorSelection();
    return UpdateColumnSelection(context.editor, 1);
}

bool ExecuteCursorColumnSelectUp(const ActionContext& context) {
    ResetInsertCursorSelection();
    return UpdateColumnSelection(context.editor, -1);
}
