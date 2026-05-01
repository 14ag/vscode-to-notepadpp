# VS Code Windows Keybinding Coverage

Generated from https://code.visualstudio.com/docs/reference/default-keybindings.

| Metric | Count |
| --- | ---: |
| Total Windows-default bindings | 163 |
| Mapped | 86 |
| Reserved no-op | 37 |
| Documented unported | 40 |

## Basic Editing

| Key | Command | VS Code command id | Status | Handling | Notes |
| --- | --- | --- | --- | --- | --- |
| `Ctrl+X` | Cut line (empty selection) | `editor.action.clipboardCutAction` | mapped | custom | Cut whole line when selection is empty, like VS Code. |
| `Ctrl+C` | Copy line (empty selection) | `editor.action.clipboardCopyAction` | mapped | custom | Copy whole line when selection is empty, like VS Code. |
| `Ctrl+V` | Paste | `editor.action.clipboardPasteAction` | mapped | nativePassThrough | Notepad++ already matches VS Code for paste. |
| `Ctrl+Shift+K` | Delete Line | `editor.action.deleteLines` | mapped | sciCommand | Delete current line when selection is empty, matching VS Code behavior closely. |
| `Ctrl+Enter` | Insert Line Below | `editor.action.insertLineAfter` | mapped | custom | Insert blank line below caret. |
| `Ctrl+Shift+Enter` | Insert Line Above | `editor.action.insertLineBefore` | mapped | custom | Insert blank line above caret. |
| `Alt+Down` | Move Line Down | `editor.action.moveLinesDownAction` | mapped | nppCommand | Move current line or selection down. |
| `Alt+Up` | Move Line Up | `editor.action.moveLinesUpAction` | mapped | nppCommand | Move current line or selection up. |
| `Shift+Alt+Down` | Copy Line Down | `editor.action.copyLinesDownAction` | mapped | nppCommand | Duplicate line downward. |
| `Shift+Alt+Up` | Copy Line Up | `editor.action.copyLinesUpAction` | mapped | custom | Duplicate line, then move duplicate upward. |
| `Ctrl+Z` | Undo | `undo` | mapped | nativePassThrough | Notepad++ already matches VS Code for undo. |
| `Ctrl+Y` | Redo | `redo` | mapped | nativePassThrough | Notepad++ already matches VS Code for redo. |
| `Ctrl+D` | Add Selection To Next Find Match | `editor.action.addSelectionToNextFindMatch` | mapped | nppCommand | Add next match to multiselection. |
| `Ctrl+K Ctrl+D` | Move Last Selection To Next Find Match | `editor.action.moveSelectionToNextFindMatch` | mapped | nppCommand | Skip current occurrence in multi-select flow. |
| `Ctrl+U` | Undo last cursor operation | `cursorUndo` | mapped | nppCommand | Undo last multi-cursor selection step. |
| `Shift+Alt+I` | Insert cursor at end of each line selected | `editor.action.insertCursorAtEndOfEachLineSelected` | reserved-noop | reservedNoOp | Multiple-caret insertion per line is not implemented safely in Notepad++. |
| `Ctrl+Shift+L` | Select all occurrences of current selection | `editor.action.selectHighlights` | mapped | nppCommand | Select all current selection matches. |
| `Ctrl+F2` | Select all occurrences of current word | `editor.action.changeAll` | mapped | nppCommand | Approximate current-word multi-select. |
| `Ctrl+L` | Select current line | `expandLineSelection` | mapped | custom | Select current line without trailing newline. |
| `Ctrl+Alt+Down` | Insert Cursor Below | `editor.action.insertCursorBelow` | mapped | custom | Move with the shared vertical-cursor rules; add/select one caret below. Virtual-space alignment is configurable. |
| `Ctrl+Alt+Up` | Insert Cursor Above | `editor.action.insertCursorAbove` | mapped | custom | Move with the shared vertical-cursor rules; add/select one caret above. |
| `Ctrl+Shift+\` | Jump to matching bracket | `editor.action.jumpToBracket` | mapped | nppCommand | Jump to matching brace. |
| `Ctrl+]` | Indent Line | `editor.action.indentLines` | mapped | nppCommand | Approximate VS Code indent with Notepad++ indent command. |
| `Ctrl+[` | Outdent Line | `editor.action.outdentLines` | mapped | nppCommand | Approximate VS Code outdent with Notepad++ unindent command. |
| `Home` | Go to Beginning of Line | `cursorHome` | mapped | nativePassThrough | Notepad++ already matches VS Code for line-home movement. |
| `End` | Go to End of Line | `cursorEnd` | mapped | nativePassThrough | Notepad++ already matches VS Code for line-end movement. |
| `Ctrl+End` | Go to End of File | `cursorBottom` | mapped | sciCommand | Jump to document end. |
| `Ctrl+Home` | Go to Beginning of File | `cursorTop` | mapped | sciCommand | Jump to document start. |
| `Ctrl+Down` | Scroll Line Down | `scrollLineDown` | mapped | sciCommand | Scroll without moving caret. |
| `Ctrl+Up` | Scroll Line Up | `scrollLineUp` | mapped | sciCommand | Scroll without moving caret. |
| `Alt+PageDown` | Scroll Page Down | `scrollPageDown` | mapped | custom | Scroll a page without moving caret. |
| `Alt+PageUp` | Scroll Page Up | `scrollPageUp` | mapped | custom | Scroll a page without moving caret. |
| `Ctrl+Shift+[` | Fold (collapse) region | `editor.fold` | mapped | nppCommand | Fold current region. |
| `Ctrl+Shift+]` | Unfold (uncollapse) region | `editor.unfold` | mapped | nppCommand | Unfold current region. |
| `Ctrl+K Ctrl+L` | Toggle Fold region | `editor.toggleFold` | mapped | custom | Toggle fold header at caret, or nearest fold parent. |
| `Ctrl+K Ctrl+[` | Fold (collapse) all subregions | `editor.foldRecursively` | reserved-noop | reservedNoOp | Reserved chord; exact recursive fold not implemented. |
| `Ctrl+K Ctrl+]` | Unfold (uncollapse) all subregions | `editor.unfoldRecursively` | reserved-noop | reservedNoOp | Reserved chord; exact recursive unfold not implemented. |
| `Ctrl+K Ctrl+0` | Fold (collapse) all regions | `editor.foldAll` | mapped | nppCommand | Fold all regions. |
| `Ctrl+K Ctrl+J` | Unfold (uncollapse) all regions | `editor.unfoldAll` | mapped | nppCommand | Unfold all regions. |
| `Ctrl+K Ctrl+C` | Add Line Comment | `editor.action.addCommentLine` | mapped | nppCommand | Force comment selected lines. |
| `Ctrl+K Ctrl+U` | Remove Line Comment | `editor.action.removeCommentLine` | mapped | nppCommand | Force uncomment selected lines. |
| `Ctrl+/` | Toggle Line Comment | `editor.action.commentLine` | mapped | custom | Toggle line-style stream comment based on current selection state. |
| `Shift+Alt+A` | Toggle Block Comment | `editor.action.blockComment` | mapped | nppCommand | Use Notepad++ block comment command. |
| `Ctrl+F` | Find | `actions.find` | mapped | nativePassThrough | Notepad++ already matches VS Code for find. |
| `Ctrl+H` | Replace | `editor.action.startFindReplaceAction` | mapped | nativePassThrough | Notepad++ already matches VS Code for replace. |
| `Enter` | Find Next | `editor.action.nextMatchFindAction` | documented-unported | documentedUnported |  |
| `Shift+Enter` | Find Previous | `editor.action.previousMatchFindAction` | documented-unported | documentedUnported |  |
| `Alt+Enter` | Select All Occurrences of Find Match | `editor.action.selectAllMatches` | mapped | nppCommand | Select all find matches. |
| `Alt+C` | Toggle Find Case Sensitive | `toggleFindCaseSensitive` | documented-unported | documentedUnported |  |
| `Alt+R` | Toggle Find Regex | `toggleFindRegex` | documented-unported | documentedUnported |  |
| `Alt+W` | Toggle Find Whole Word | `toggleFindWholeWord` | documented-unported | documentedUnported |  |
| `Ctrl+M` | Toggle Use of Tab Key for Setting Focus | `editor.action.toggleTabFocusMode` | documented-unported | documentedUnported |  |
| `Alt+Z` | Toggle Word Wrap | `editor.action.toggleWordWrap` | mapped | nppCommand | Toggle word wrap. |
| `Ctrl+Alt+Shift+Down` | Column Select Down | `editor.action.cursorColumnSelectDown` | mapped | custom | Move with the shared vertical-cursor rules; rebuild the anchored column-selection range downward. Undocumented on the VS Code default keybindings page, but available in the VS Code Keyboard Shortcuts editor. |
| `Ctrl+Alt+Shift+Up` | Column Select Up | `editor.action.cursorColumnSelectUp` | mapped | custom | Move with the shared vertical-cursor rules; rebuild the anchored column-selection range upward. Undocumented on the VS Code default keybindings page, but available in the VS Code Keyboard Shortcuts editor. |
## Rich Languages Editing

| Key | Command | VS Code command id | Status | Handling | Notes |
| --- | --- | --- | --- | --- | --- |
| `Ctrl+Space` | Trigger Suggest | `editor.action.triggerSuggest` | mapped | nppCommand | Open autocomplete. |
| `Ctrl+Shift+Space` | Trigger Parameter Hints | `editor.action.triggerParameterHints` | mapped | nppCommand | Open function call tip. |
| `Shift+Alt+F` | Format Document | `editor.action.formatDocument` | reserved-noop | reservedNoOp | Formatting depends on plugins/language support, so strict mode reserves it. |
| `Ctrl+K Ctrl+F` | Format Selection | `editor.action.formatSelection` | reserved-noop | reservedNoOp | Reserved so conflicting Notepad++ behavior does not fire for VS Code format-selection chord. |
| `F12` | Go to Definition | `editor.action.revealDefinition` | reserved-noop | reservedNoOp | No built-in go-to-definition equivalent. |
| `Ctrl+K Ctrl+I` | Show Hover | `editor.action.showHover` | reserved-noop | reservedNoOp | Reserved so conflicting Notepad++ behavior does not fire for VS Code hover chord. |
| `Alt+F12` | Peek Definition | `editor.action.peekDefinition` | reserved-noop | reservedNoOp | No peek-definition equivalent. |
| `Ctrl+K F12` | Open Definition to the Side | `editor.action.revealDefinitionAside` | reserved-noop | reservedNoOp | No open-definition-side equivalent. |
| `Ctrl+.` | Quick Fix | `editor.action.quickFix` | reserved-noop | reservedNoOp | No quick-fix provider equivalent. |
| `Shift+F12` | Go to References | `editor.action.goToReferences` | reserved-noop | reservedNoOp | No find-references equivalent. |
| `F2` | Rename Symbol | `editor.action.rename` | reserved-noop | reservedNoOp | No language-aware rename equivalent. |
| `Ctrl+Shift+.` | Replace with Next Value | `editor.action.inPlaceReplace.down` | documented-unported | documentedUnported |  |
| `Ctrl+Shift+,` | Replace with Previous Value | `editor.action.inPlaceReplace.up` | documented-unported | documentedUnported |  |
| `Shift+Alt+Right` | Expand AST Selection | `editor.action.smartSelect.expand` | documented-unported | documentedUnported |  |
| `Shift+Alt+Left` | Shrink AST Selection | `editor.action.smartSelect.shrink` | documented-unported | documentedUnported |  |
| `Ctrl+K Ctrl+X` | Trim Trailing Whitespace | `editor.action.trimTrailingWhitespace` | mapped | nppCommand | Trim trailing whitespace. |
| `Ctrl+K M` | Change Language Mode | `workbench.action.editor.changeLanguageMode` | reserved-noop | reservedNoOp | Reserved so conflicting Notepad++ behavior does not fire for VS Code language-mode chord. |
## Navigation

| Key | Command | VS Code command id | Status | Handling | Notes |
| --- | --- | --- | --- | --- | --- |
| `Ctrl+T` | Show All Symbols | `workbench.action.showAllSymbols` | mapped | nppCommand | Approximate workspace symbol search with function list. |
| `Ctrl+G` | Go to Line... | `workbench.action.gotoLine` | mapped | nppCommand | Go to line. |
| `Ctrl+P` | Go to File..., Quick Open | `workbench.action.quickOpen` | mapped | nppCommand | Approximate Quick Open with file-open dialog. |
| `Ctrl+Shift+O` | Go to Symbol... | `workbench.action.gotoSymbol` | mapped | nppCommand | Approximate symbol picker with function list panel. |
| `Ctrl+Shift+M` | Show Problems | `workbench.actions.view.problems` | reserved-noop | reservedNoOp | Reserved so conflicting Notepad++ behavior does not fire for VS Code problems shortcut. |
| `F8` | Go to Next Error or Warning | `editor.action.marker.nextInFiles` | reserved-noop | reservedNoOp | No Problems panel equivalent. |
| `Shift+F8` | Go to Previous Error or Warning | `editor.action.marker.prevInFiles` | reserved-noop | reservedNoOp | No Problems panel equivalent. |
| `Ctrl+Shift+P` | Show All Commands | `workbench.action.showCommands` | reserved-noop | reservedNoOp | No Command Palette equivalent. |
| `Ctrl+Tab` | Navigate Editor Group History | `workbench.action.quickOpenPreviousRecentlyUsedEditorInGroup` | mapped | nppCommand | Cycle tabs forward. |
| `Alt+Left` | Go Back | `workbench.action.navigateBack` | reserved-noop | reservedNoOp | No stable back-stack equivalent. |
| `Alt+Left` | Go back in Quick Input | `workbench.action.quickInputBack` | documented-unported | documentedUnported |  |
| `Alt+Right` | Go Forward | `workbench.action.navigateForward` | reserved-noop | reservedNoOp | No stable forward-stack equivalent. |
| `Ctrl+Shift+;` | Focus Breadcrumbs | `breadcrumbs.focus` | documented-unported | documentedUnported |  |
| `Ctrl+Shift+.` | Focus and Select Breadcrumbs | `breadcrumbs.focusAndSelect` | documented-unported | documentedUnported |  |
## Editor/Window Management

| Key | Command | VS Code command id | Status | Handling | Notes |
| --- | --- | --- | --- | --- | --- |
| `Ctrl+Shift+N` | New Window | `workbench.action.newWindow` | mapped | nppCommand | Approximate new window with new Notepad++ instance. |
| `Alt+F4` | Close Window | `workbench.action.closeWindow` | documented-unported | documentedUnported |  |
| `Ctrl+F4` | Close Editor | `workbench.action.closeActiveEditor` | mapped | nppCommand | Close active file. |
| `Ctrl+K F` | Close Folder | `workbench.action.closeFolder` | reserved-noop | reservedNoOp | Reserved so conflicting Notepad++ behavior does not fire for VS Code close-folder chord. |
| `Ctrl+\` | Split Editor | `workbench.action.splitEditor` | mapped | nppCommand | Approximate split editor by moving document to the other view. |
| `Ctrl+1` | Focus into First Editor Group | `workbench.action.focusFirstEditorGroup` | mapped | nppCommand | Approximate first editor group focus with first tab selection. |
| `Ctrl+2` | Focus into Second Editor Group | `workbench.action.focusSecondEditorGroup` | mapped | nppCommand | Approximate second editor group focus with second tab selection. |
| `Ctrl+3` | Focus into Third Editor Group | `workbench.action.focusThirdEditorGroup` | mapped | nppCommand | Approximate third editor group focus with third tab selection. |
| `Ctrl+K Ctrl+Left` | Focus into Editor Group on the Left | `workbench.action.focusLeftGroup` | reserved-noop | reservedNoOp | Reserved so conflicting Notepad++ behavior does not fire for VS Code focus-group chord. |
| `Ctrl+K Ctrl+Right` | Focus into Editor Group on the Right | `workbench.action.focusRightGroup` | reserved-noop | reservedNoOp | Reserved so conflicting Notepad++ behavior does not fire for VS Code focus-group chord. |
| `Ctrl+Shift+PageUp` | Move Editor Left | `workbench.action.moveEditorLeftInGroup` | mapped | nppCommand | Approximate move editor left with move tab left. |
| `Ctrl+Shift+PageDown` | Move Editor Right | `workbench.action.moveEditorRightInGroup` | mapped | nppCommand | Approximate move editor right with move tab right. |
| `Ctrl+K Left` | Move Active Editor Group Left | `workbench.action.moveActiveEditorGroupLeft` | reserved-noop | reservedNoOp | Reserved so conflicting Notepad++ behavior does not fire for VS Code move-group chord. |
| `Ctrl+K Right` | Move Active Editor Group Right | `workbench.action.moveActiveEditorGroupRight` | reserved-noop | reservedNoOp | Reserved so conflicting Notepad++ behavior does not fire for VS Code move-group chord. |
| `Ctrl+Alt+Right` | Move Editor into Next Group | `workbench.action.moveEditorToNextGroup` | mapped | nppCommand | Approximate move editor to next group by moving document to the other view. |
| `Ctrl+Alt+Left` | Move Editor into Previous Group | `workbench.action.moveEditorToPreviousGroup` | mapped | nppCommand | Approximate move editor to previous group by moving document to the other view. |
## File Management

| Key | Command | VS Code command id | Status | Handling | Notes |
| --- | --- | --- | --- | --- | --- |
| `Ctrl+N` | New File | `workbench.action.files.newUntitledFile` | mapped | nativePassThrough | Notepad++ already matches VS Code for new file. |
| `Ctrl+O` | Open File... | `workbench.action.files.openFile` | mapped | nativePassThrough | Notepad++ already matches VS Code for open file. |
| `Ctrl+S` | Save | `workbench.action.files.save` | mapped | nativePassThrough | Notepad++ already matches VS Code for save. |
| `Ctrl+K S` | Save All | `saveAll` | mapped | nppCommand | Save all open files. |
| `Ctrl+Shift+S` | Save As... | `workbench.action.files.saveAs` | mapped | nppCommand | Save current file as. |
| `Ctrl+K W` | Close Group | `workbench.action.closeEditorsInGroup` | reserved-noop | reservedNoOp | Reserved so conflicting Notepad++ behavior does not fire for VS Code close-group chord. |
| `Ctrl+K Ctrl+W` | Close All | `workbench.action.closeAllEditors` | mapped | nppCommand | Close all files. |
| `Ctrl+Shift+T` | Reopen Closed Editor | `workbench.action.reopenClosedEditor` | mapped | nppCommand | Restore last closed file. |
| `Ctrl+K Enter` | Keep Open | `workbench.action.keepEditor` | reserved-noop | reservedNoOp | Reserved so conflicting Notepad++ behavior does not fire for VS Code keep-open chord. |
| `Ctrl+K P` | Copy Path of Active File | `workbench.action.files.copyPathOfActiveFile` | mapped | nppCommand | Copy active file path. |
| `Ctrl+K R` | Reveal Active File in Windows | `workbench.action.files.revealActiveFileInWindows` | mapped | nppCommand | Reveal active file in Explorer. |
## Display

| Key | Command | VS Code command id | Status | Handling | Notes |
| --- | --- | --- | --- | --- | --- |
| `F11` | Toggle Full Screen | `workbench.action.toggleFullScreen` | mapped | nppCommand | Toggle fullscreen. |
| `Ctrl+K Z` | Toggle Zen Mode | `workbench.action.toggleZenMode` | mapped | nppCommand | Approximate VS Code Zen Mode with Notepad++ distraction-free mode. |
| `Escape Escape` | Leave Zen Mode | `workbench.action.exitZenMode` | documented-unported | documentedUnported |  |
| `Ctrl+=` | Zoom in | `workbench.action.zoomIn` | mapped | nppCommand | Zoom in. |
| `Ctrl+-` | Zoom out | `workbench.action.zoomOut` | mapped | nppCommand | Zoom out. |
| `Ctrl+Numpad0` | Reset Zoom | `workbench.action.zoomReset` | mapped | nppCommand | Reset zoom. |
| `Ctrl+B` | Toggle Sidebar Visibility | `workbench.action.toggleSidebarVisibility` | mapped | nppCommand | Approximate sidebar toggle with file browser panel. |
| `Ctrl+Shift+E` | Show Explorer / Toggle Focus | `workbench.view.explorer` | mapped | nppCommand | Focus file browser panel. |
| `Ctrl+Shift+F` | Show Search | `workbench.view.search` | mapped | nppCommand | Find in files. |
| `Ctrl+Shift+G` | Show Source Control | `workbench.view.scm` | reserved-noop | reservedNoOp | No source-control sidebar equivalent. |
| `Ctrl+Shift+D` | Show Run | `workbench.view.debug` | reserved-noop | reservedNoOp | No Run/Debug sidebar equivalent. |
| `Ctrl+Shift+X` | Show Extensions | `workbench.view.extensions` | reserved-noop | reservedNoOp | No extensions marketplace sidebar equivalent. |
| `Ctrl+Shift+U` | Show Output | `workbench.action.output.toggleOutput` | reserved-noop | reservedNoOp | No Output panel equivalent. |
| `Ctrl+Q` | Quick Open View | `workbench.action.quickOpenView` | reserved-noop | reservedNoOp | Reserved so conflicting Notepad++ behavior does not fire for VS Code quick-open-view shortcut. |
| `Ctrl+Shift+C` | Open New Command Prompt | `workbench.action.terminal.openNativeConsole` | mapped | nppCommand | Approximate with Open Command Prompt Here. |
| `Ctrl+Shift+V` | Toggle Markdown Preview | `markdown.showPreview` | reserved-noop | reservedNoOp | Reserved so conflicting Notepad++ behavior does not fire for VS Code markdown-preview shortcut. |
| `Ctrl+K V` | Open Preview to the Side | `markdown.showPreviewToSide` | reserved-noop | reservedNoOp | Reserved so conflicting Notepad++ behavior does not fire for VS Code markdown-preview chord. |
| `Ctrl+`` | Toggle Integrated Terminal | `workbench.action.terminal.toggleTerminal` | mapped | nppCommand | Approximate integrated terminal with Open Command Prompt Here. |
## Search

| Key | Command | VS Code command id | Status | Handling | Notes |
| --- | --- | --- | --- | --- | --- |
| `Ctrl+Shift+H` | Replace in Files | `workbench.action.replaceInFiles` | mapped | nppCommand | Approximate replace-in-files with Notepad++ find-in-files panel. |
| `Alt+C` | Toggle Match Case | `toggleSearchCaseSensitive` | documented-unported | documentedUnported |  |
| `Alt+W` | Toggle Match Whole Word | `toggleSearchWholeWord` | documented-unported | documentedUnported |  |
| `Alt+R` | Toggle Use Regular Expression | `toggleSearchRegex` | documented-unported | documentedUnported |  |
| `Ctrl+Shift+J` | Toggle Search Details | `workbench.action.search.toggleQueryDetails` | reserved-noop | reservedNoOp | Reserved so conflicting Notepad++ behavior does not fire for VS Code search-details shortcut. |
| `F4` | Focus Next Search Result | `search.action.focusNextSearchResult` | documented-unported | documentedUnported |  |
| `Shift+F4` | Focus Previous Search Result | `search.action.focusPreviousSearchResult` | documented-unported | documentedUnported |  |
| `Down` | Show Next Search Term | `history.showNext` | documented-unported | documentedUnported |  |
| `Up` | Show Previous Search Term | `history.showPrevious` | documented-unported | documentedUnported |  |
## Search Editor

| Key | Command | VS Code command id | Status | Handling | Notes |
| --- | --- | --- | --- | --- | --- |
| `Alt+Enter` | Open Results In Editor | `search.action.openInEditor` | documented-unported | documentedUnported |  |
| `Escape` | Focus Search Editor Input | `search.action.focusQueryEditorWidget` | documented-unported | documentedUnported |  |
| `Ctrl+Shift+R` | Search Again | `rerunSearchEditorSearch` | documented-unported | documentedUnported |  |
| `Ctrl+Shift+Backspace` | Delete File Results | `search.searchEditor.action.deleteFileResults` | documented-unported | documentedUnported |  |
## Preferences

| Key | Command | VS Code command id | Status | Handling | Notes |
| --- | --- | --- | --- | --- | --- |
| `Ctrl+,` | Open Settings | `workbench.action.openSettings` | mapped | nppCommand | Open Notepad++ preferences. |
| `Ctrl+K Ctrl+S` | Open Keyboard Shortcuts | `workbench.action.openGlobalKeybindings` | mapped | nppCommand | Open Shortcut Mapper. |
| `Ctrl+K Ctrl+T` | Select Color Theme | `workbench.action.selectTheme` | reserved-noop | reservedNoOp | Reserved so conflicting Notepad++ behavior does not fire for VS Code theme chord. |
## Chat

| Key | Command | VS Code command id | Status | Handling | Notes |
| --- | --- | --- | --- | --- | --- |
| `Ctrl+Alt+I` | Open Chat view | `workbench.action.chat.open` | documented-unported | documentedUnported |  |
| `Ctrl+Shift+I` | Open chat in agent mode | `workbench.action.chat.openagent` | documented-unported | documentedUnported |  |
| `Ctrl+I` | Open editor inline chat | `inlineChat.start` | documented-unported | documentedUnported |  |
| `Ctrl+I` | Open terminal inline chat | `workbench.action.terminal.chat.start` | documented-unported | documentedUnported |  |
| `Ctrl+Shift+Alt+L` | Open quick chat | `workbench.action.quickchat.toggle` | documented-unported | documentedUnported |  |
| `Ctrl+.` | Open agent picker | `workbench.action.chat.openModePicker` | documented-unported | documentedUnported |  |
| `Ctrl+Alt+.` | Open language model picker | `workbench.action.chat.openModelPicker` | documented-unported | documentedUnported |  |
| `Ctrl+N` | New chat session | `workbench.action.chat.newChat` | documented-unported | documentedUnported |  |
| `Tab` | Accept inline suggestion | `editor.action.inlineSuggest.commit` | documented-unported | documentedUnported |  |
## Debug

| Key | Command | VS Code command id | Status | Handling | Notes |
| --- | --- | --- | --- | --- | --- |
| `F9` | Toggle Breakpoint | `editor.debug.action.toggleBreakpoint` | reserved-noop | reservedNoOp | Reserved so conflicting Notepad++ behavior does not fire for VS Code breakpoint shortcut. |
| `F5` | Start | `workbench.action.debug.start` | reserved-noop | reservedNoOp | Reserved so conflicting Notepad++ behavior does not fire for VS Code debug-start shortcut. |
| `F5` | Continue | `workbench.action.debug.continue` | documented-unported | documentedUnported |  |
| `Ctrl+F5` | Start (without debugging) | `workbench.action.debug.run` | documented-unported | documentedUnported |  |
| `F6` | Pause | `workbench.action.debug.pause` | documented-unported | documentedUnported |  |
| `F11` | Step Into | `workbench.action.debug.stepInto` | documented-unported | documentedUnported |  |
## Tasks

| Key | Command | VS Code command id | Status | Handling | Notes |
| --- | --- | --- | --- | --- | --- |
| `Ctrl+Shift+B` | Run Build Task | `workbench.action.tasks.build` | documented-unported | documentedUnported |  |

Status meanings: mapped = VS Code shortcut covered, eserved-noop = swallowed to avoid bad fallthrough, documented-unported = listed but intentionally not emulated.
