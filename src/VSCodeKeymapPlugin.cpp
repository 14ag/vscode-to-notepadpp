#include <windows.h>
#include <commctrl.h>

#include <algorithm>
#include <cwchar>
#include <cwctype>
#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "Notepad_plus_msgs.h"
#include "PluginInterface.h"
#include "handlers/ActionHandlers.h"

namespace {
constexpr wchar_t kPluginName[] = L"VSCode Keymap NPP";
constexpr DWORD kChordTimeoutMs = 1500;
constexpr int kConfigDialogId = 101;
constexpr int kEnableKeymapCheckboxId = 1001;
constexpr int kReferenceFilterEditId = 1002;
constexpr int kBindingsListId = 1003;
constexpr int kReferenceSummaryId = 1004;
constexpr wchar_t kSettingsSectionGeneral[] = L"General";
constexpr wchar_t kSettingsSectionBindings[] = L"Bindings";
constexpr wchar_t kSettingsKeyEnabled[] = L"KeymapEnabled";
constexpr wchar_t kLegacySettingsKeyPluginDisabled[] = L"PluginDisabled";
constexpr wchar_t kSettingsKeyDisabledBindings[] = L"DisabledBindings";

NppData g_nppData{};
FuncItem g_funcItems[2]{};
bool g_enabled = true;
bool g_syncingBindingList = false;
size_t g_shortcutHandlingPauseDepth = 0;
std::vector<bool> g_bindingEnabled;

WNDPROC g_mainOldProc = nullptr;
WNDPROC g_scintillaOldProc[2] = {nullptr, nullptr};
HHOOK g_messageHook = nullptr;
DWORD g_nppThreadId = 0;

struct PendingChord {
    KeyStroke first;
    DWORD startedAt;
};

std::optional<PendingChord> g_pendingChord;

#include "GeneratedBindings.inc"

constexpr unsigned long long kBindingStorageHashOffset = 14695981039346656037ull;
constexpr unsigned long long kBindingStorageHashPrime = 1099511628211ull;

extern "C" IMAGE_DOS_HEADER __ImageBase;

class ScopedShortcutHandlingPause {
public:
    ScopedShortcutHandlingPause() {
        ++g_shortcutHandlingPauseDepth;
        g_pendingChord.reset();
    }

    ~ScopedShortcutHandlingPause() {
        if (g_shortcutHandlingPauseDepth > 0) {
            --g_shortcutHandlingPauseDepth;
        }
        g_pendingChord.reset();
    }

    ScopedShortcutHandlingPause(const ScopedShortcutHandlingPause&) = delete;
    ScopedShortcutHandlingPause& operator=(const ScopedShortcutHandlingPause&) = delete;
};

HINSTANCE ModuleInstance() {
    return reinterpret_cast<HINSTANCE>(&__ImageBase);
}

void ResetBindingStatesToDefaults() {
    g_bindingEnabled.assign(kBindingReferences.size(), false);
    for (size_t index = 0; index < kBindingReferences.size(); ++index) {
        g_bindingEnabled[index] = kBindingReferences[index].pluginHandled;
    }
}

std::filesystem::path SettingsFilePath() {
    if (g_nppData._nppHandle == nullptr) {
        return {};
    }

    std::vector<wchar_t> buffer(MAX_PATH, L'\0');
    const auto copied = static_cast<size_t>(::SendMessage(g_nppData._nppHandle,
                                                          NPPM_GETPLUGINSCONFIGDIR,
                                                          static_cast<WPARAM>(buffer.size()),
                                                          reinterpret_cast<LPARAM>(buffer.data())));
    if (copied == 0 || buffer[0] == L'\0') {
        return {};
    }

    return std::filesystem::path(buffer.data()) / L"VSCodeKeymapNpp.ini";
}

std::wstring SettingsFilePathText() {
    const auto path = SettingsFilePath();
    if (path.empty()) {
        return L"(Notepad++ plugin config directory unavailable)";
    }
    return path.wstring();
}

std::wstring GetWindowTextValue(HWND control) {
    const int length = ::GetWindowTextLengthW(control);
    if (length <= 0) {
        return {};
    }

    std::wstring text(static_cast<size_t>(length) + 1, L'\0');
    ::GetWindowTextW(control, text.data(), length + 1);
    text.resize(static_cast<size_t>(length));
    return text;
}

unsigned long long HashBindingStorageComponent(unsigned long long hash, std::wstring_view value) {
    for (const wchar_t ch : value) {
        hash ^= static_cast<unsigned long long>(static_cast<unsigned int>(ch));
        hash *= kBindingStorageHashPrime;
    }
    hash ^= 0xffull;
    hash *= kBindingStorageHashPrime;
    return hash;
}

unsigned long long BindingStorageHash(const BindingReferenceEntry& binding) {
    auto hash = kBindingStorageHashOffset;
    hash = HashBindingStorageComponent(hash, binding.command ? binding.command : L"");
    hash = HashBindingStorageComponent(hash, binding.winKey ? binding.winKey : L"");
    return hash;
}

std::wstring BuildBindingStorageToken(const BindingReferenceEntry& binding) {
    wchar_t token[19]{};
    std::swprintf(token, std::size(token), L"h:%016llx", BindingStorageHash(binding));
    return token;
}

std::optional<size_t> FindBindingIndexFromPersistedToken(std::wstring_view token) {
    std::wstring tokenText(token);
    if (tokenText.empty()) {
        return std::nullopt;
    }

    if (tokenText.rfind(L"h:", 0) == 0) {
        wchar_t* parsedEnd = nullptr;
        const auto parsedHash = std::wcstoull(tokenText.c_str() + 2, &parsedEnd, 16);
        if (parsedEnd != tokenText.c_str() + 2 && *parsedEnd == L'\0') {
            for (size_t index = 0; index < kBindingReferences.size(); ++index) {
                if (kBindingReferences[index].pluginHandled && BindingStorageHash(kBindingReferences[index]) == parsedHash) {
                    return index;
                }
            }
        }
        return std::nullopt;
    }

    wchar_t* parsedEnd = nullptr;
    const auto legacyIndex = std::wcstoul(tokenText.c_str(), &parsedEnd, 10);
    if (parsedEnd == tokenText.c_str() || *parsedEnd != L'\0' || legacyIndex >= kBindingReferences.size()) {
        return std::nullopt;
    }

    return static_cast<size_t>(legacyIndex);
}

std::wstring BuildDisabledBindingList() {
    std::wstring value;
    for (size_t index = 0; index < kBindingReferences.size(); ++index) {
        if (!kBindingReferences[index].pluginHandled || g_bindingEnabled[index]) {
            continue;
        }
        if (!value.empty()) {
            value += L",";
        }
        value += BuildBindingStorageToken(kBindingReferences[index]);
    }
    return value;
}

void ApplyDisabledBindingList(const std::wstring& value) {
    std::wstringstream stream(value);
    std::wstring token;
    while (std::getline(stream, token, L',')) {
        if (token.empty()) {
            continue;
        }

        const auto index = FindBindingIndexFromPersistedToken(token);
        if (!index.has_value()) {
            continue;
        }
        if (kBindingReferences[*index].pluginHandled) {
            g_bindingEnabled[*index] = false;
        }
    }
}

void SaveSettings() {
    const auto settingsPath = SettingsFilePath();
    if (settingsPath.empty()) {
        return;
    }

    std::filesystem::create_directories(settingsPath.parent_path());
    ::WritePrivateProfileStringW(
        kSettingsSectionGeneral, kSettingsKeyEnabled, g_enabled ? L"1" : L"0", settingsPath.c_str());
    ::WritePrivateProfileStringW(kSettingsSectionGeneral, kLegacySettingsKeyPluginDisabled, nullptr, settingsPath.c_str());

    const auto disabledBindings = BuildDisabledBindingList();
    ::WritePrivateProfileStringW(kSettingsSectionBindings,
                                 kSettingsKeyDisabledBindings,
                                 disabledBindings.empty() ? L"" : disabledBindings.c_str(),
                                 settingsPath.c_str());
}

void LoadSettings() {
    ResetBindingStatesToDefaults();

    const auto settingsPath = SettingsFilePath();
    if (settingsPath.empty()) {
        return;
    }

    g_enabled = ::GetPrivateProfileIntW(kSettingsSectionGeneral, kSettingsKeyEnabled, 1, settingsPath.c_str()) != 0;
    const bool legacyPluginDisabled =
        ::GetPrivateProfileIntW(kSettingsSectionGeneral, kLegacySettingsKeyPluginDisabled, 0, settingsPath.c_str()) !=
        0;
    g_enabled = g_enabled && !legacyPluginDisabled;

    std::wstring disabledBindings(4096, L'\0');
    const auto length = ::GetPrivateProfileStringW(kSettingsSectionBindings,
                                                   kSettingsKeyDisabledBindings,
                                                   L"",
                                                   disabledBindings.data(),
                                                   static_cast<DWORD>(disabledBindings.size()),
                                                   settingsPath.c_str());
    disabledBindings.resize(length);
    ApplyDisabledBindingList(disabledBindings);
}

bool IsBindingConfigurable(size_t referenceIndex) {
    return referenceIndex < kBindingReferences.size() && kBindingReferences[referenceIndex].pluginHandled;
}

bool IsBindingEnabled(size_t referenceIndex) {
    return IsBindingConfigurable(referenceIndex) && g_bindingEnabled[referenceIndex];
}

void SetBindingEnabled(size_t referenceIndex, bool enabled) {
    if (!IsBindingConfigurable(referenceIndex)) {
        return;
    }

    g_bindingEnabled[referenceIndex] = enabled;
    g_pendingChord.reset();
    SaveSettings();
}

std::wstring BuildReferenceSummaryText() {
    const auto enabledCount = static_cast<size_t>(std::count(g_bindingEnabled.begin(), g_bindingEnabled.end(), true));
    const auto configurableCount = static_cast<size_t>(
        std::count_if(kBindingReferences.begin(), kBindingReferences.end(), [](const auto& binding) {
            return binding.pluginHandled;
        }));

    std::wstring text;
    if (g_enabled) {
        text = L"Mode: intercepting selected VS Code shortcuts";
    } else {
        text = L"Mode: pass-through (saved selections are preserved)";
    }
    text += L"\r\nChecked rows: ";
    text += std::to_wstring(enabledCount);
    text += L" / ";
    text += std::to_wstring(configurableCount);
    text += L"    Settings file: ";
    text += SettingsFilePathText();
    return text;
}

std::wstring BuildSectionLabel(const BindingReferenceEntry& entry) {
    std::wstring value = entry.section ? entry.section : L"";
    if (entry.subsection != nullptr && entry.subsection[0] != L'\0') {
        if (!value.empty()) {
            value += L" / ";
        }
        value += entry.subsection;
    }
    return value;
}

bool ContainsCaseInsensitive(std::wstring_view text, std::wstring_view needle) {
    if (needle.empty()) {
        return true;
    }

    const auto match = std::search(text.begin(),
                                   text.end(),
                                   needle.begin(),
                                   needle.end(),
                                   [](wchar_t lhs, wchar_t rhs) { return std::towlower(lhs) == std::towlower(rhs); });
    return match != text.end();
}

bool BindingMatchesFilter(const BindingReferenceEntry& binding, std::wstring_view filter) {
    if (filter.empty()) {
        return true;
    }

    const auto section = BuildSectionLabel(binding);
    return ContainsCaseInsensitive(binding.winKey ? binding.winKey : L"", filter) ||
           ContainsCaseInsensitive(section, filter) ||
           ContainsCaseInsensitive(binding.label ? binding.label : L"", filter) ||
           ContainsCaseInsensitive(binding.command ? binding.command : L"", filter) ||
           ContainsCaseInsensitive(binding.status ? binding.status : L"", filter) ||
           ContainsCaseInsensitive(binding.handler ? binding.handler : L"", filter) ||
           ContainsCaseInsensitive(binding.target ? binding.target : L"", filter) ||
           ContainsCaseInsensitive(binding.notes ? binding.notes : L"", filter);
}

void InsertReferenceColumns(HWND listView) {
    struct ColumnSpec {
        const wchar_t* title;
        int width;
    };

    const ColumnSpec columns[] = {
        {L"Key", 120},
        {L"Section", 170},
        {L"Label", 170},
        {L"Command", 240},
        {L"Status", 110},
        {L"Handler", 120},
        {L"Target", 180},
        {L"Notes", 420},
    };

    for (int index = 0; index < static_cast<int>(std::size(columns)); ++index) {
        LVCOLUMNW column{};
        column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        column.cx = columns[index].width;
        column.pszText = const_cast<LPWSTR>(columns[index].title);
        column.iSubItem = index;
        ListView_InsertColumn(listView, index, &column);
    }
}

size_t PopulateReferenceList(HWND dialog, HWND listView) {
    const auto filter = GetWindowTextValue(::GetDlgItem(dialog, kReferenceFilterEditId));
    g_syncingBindingList = true;
    ListView_DeleteAllItems(listView);

    int visibleIndex = 0;
    for (size_t referenceIndex = 0; referenceIndex < kBindingReferences.size(); ++referenceIndex) {
        const auto& binding = kBindingReferences[referenceIndex];
        if (!BindingMatchesFilter(binding, filter)) {
            continue;
        }

        LVITEMW item{};
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = visibleIndex;
        item.lParam = static_cast<LPARAM>(referenceIndex);
        item.pszText = const_cast<LPWSTR>(binding.winKey);
        ListView_InsertItem(listView, &item);

        const auto section = BuildSectionLabel(binding);
        ListView_SetItemText(listView, visibleIndex, 1, const_cast<LPWSTR>(section.c_str()));
        ListView_SetItemText(listView, visibleIndex, 2, const_cast<LPWSTR>(binding.label));
        ListView_SetItemText(listView, visibleIndex, 3, const_cast<LPWSTR>(binding.command));
        ListView_SetItemText(listView, visibleIndex, 4, const_cast<LPWSTR>(binding.status));
        ListView_SetItemText(listView, visibleIndex, 5, const_cast<LPWSTR>(binding.handler));
        ListView_SetItemText(listView, visibleIndex, 6, const_cast<LPWSTR>(binding.target));
        ListView_SetItemText(listView, visibleIndex, 7, const_cast<LPWSTR>(binding.notes));
        ListView_SetCheckState(listView, visibleIndex, IsBindingEnabled(referenceIndex) ? TRUE : FALSE);
        ++visibleIndex;
    }

    g_syncingBindingList = false;
    return static_cast<size_t>(visibleIndex);
}

size_t RefreshReferenceList(HWND dialog) {
    const HWND listView = ::GetDlgItem(dialog, kBindingsListId);
    const size_t visibleCount = PopulateReferenceList(dialog, listView);
    ::SetDlgItemTextW(dialog, kReferenceSummaryId, BuildReferenceSummaryText().c_str());
    return visibleCount;
}

void ApplyDialogSettings(HWND dialog) {
    g_enabled = (::IsDlgButtonChecked(dialog, kEnableKeymapCheckboxId) == BST_CHECKED);
    g_pendingChord.reset();
    SaveSettings();
}

size_t GetReferenceIndexForListItem(HWND listView, int itemIndex) {
    LVITEMW item{};
    item.mask = LVIF_PARAM;
    item.iItem = itemIndex;
    item.iSubItem = 0;
    if (!ListView_GetItem(listView, &item) || item.lParam < 0) {
        return kBindingReferences.size();
    }
    return static_cast<size_t>(item.lParam);
}

INT_PTR CALLBACK ConfigReferenceDialogProc(HWND dialog, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG: {
            const HWND listView = ::GetDlgItem(dialog, kBindingsListId);
            ListView_SetExtendedListViewStyle(
                listView, LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
            InsertReferenceColumns(listView);
            RefreshReferenceList(dialog);

            ::CheckDlgButton(dialog, kEnableKeymapCheckboxId, g_enabled ? BST_CHECKED : BST_UNCHECKED);
            return TRUE;
        }

        case WM_NOTIFY: {
            auto* header = reinterpret_cast<NMHDR*>(lParam);
            if (header == nullptr || header->idFrom != kBindingsListId || header->code != LVN_ITEMCHANGED) {
                break;
            }

            auto* changed = reinterpret_cast<NMLISTVIEW*>(lParam);
            if (g_syncingBindingList || changed->iItem < 0 || (changed->uChanged & LVIF_STATE) == 0) {
                break;
            }

            const bool oldChecked = ((changed->uOldState & LVIS_STATEIMAGEMASK) >> 12) == 2;
            const bool newChecked = ListView_GetCheckState(header->hwndFrom, changed->iItem) != FALSE;
            if (oldChecked == newChecked) {
                break;
            }

            const auto referenceIndex = GetReferenceIndexForListItem(header->hwndFrom, changed->iItem);
            if (referenceIndex >= kBindingReferences.size()) {
                return TRUE;
            }

            if (!IsBindingConfigurable(referenceIndex)) {
                g_syncingBindingList = true;
                ListView_SetCheckState(header->hwndFrom, changed->iItem, FALSE);
                g_syncingBindingList = false;
                return TRUE;
            }

            SetBindingEnabled(referenceIndex, newChecked);
            ::SetDlgItemTextW(dialog, kReferenceSummaryId, BuildReferenceSummaryText().c_str());
            return TRUE;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case kEnableKeymapCheckboxId:
                    if (HIWORD(wParam) == BN_CLICKED) {
                        ApplyDialogSettings(dialog);
                        ::SetDlgItemTextW(dialog, kReferenceSummaryId, BuildReferenceSummaryText().c_str());
                    }
                    return TRUE;

                case kReferenceFilterEditId:
                    if (HIWORD(wParam) == EN_CHANGE) {
                        RefreshReferenceList(dialog);
                    }
                    return TRUE;

                case IDOK:
                case IDCANCEL:
                    ::EndDialog(dialog, LOWORD(wParam));
                    return TRUE;
            }
            break;

        case WM_CLOSE:
            ::EndDialog(dialog, IDCANCEL);
            return TRUE;
    }

    return FALSE;
}

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

KeyStroke CurrentKeyStroke(UINT vk) {
    return KeyStroke{IsPressed(VK_CONTROL), IsPressed(VK_MENU), IsPressed(VK_SHIFT), vk};
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

bool TryHandleChord(const KeyStroke& currentKey, HWND editor) {
    if (g_pendingChord.has_value()) {
        const DWORD age = ::GetTickCount() - g_pendingChord->startedAt;
        if (age <= kChordTimeoutMs) {
            bool enabledPrefixMatched = false;
            for (const auto& chord : kChordBindings) {
                if (chord.first != g_pendingChord->first || !IsBindingEnabled(chord.referenceIndex)) {
                    continue;
                }

                enabledPrefixMatched = true;
                if (chord.second == currentKey) {
                    g_pendingChord.reset();
                    return ExecuteAction(chord.kind, chord.id, ActionContext{g_nppData._nppHandle, editor});
                }
            }

            if (enabledPrefixMatched) {
                // Strict mode: consume unknown second key after a valid chord prefix.
                g_pendingChord.reset();
                return true;
            }
        }

        g_pendingChord.reset();
    }

    for (const auto& chord : kChordBindings) {
        if (chord.first == currentKey && IsBindingEnabled(chord.referenceIndex)) {
            g_pendingChord = PendingChord{currentKey, ::GetTickCount()};
            return true;
        }
    }

    return false;
}

bool TryHandleShortcut(HWND editor, WPARAM wParam) {
    if (g_shortcutHandlingPauseDepth > 0 || !g_enabled || editor == nullptr) {
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
        if (binding.key == currentKey && IsBindingEnabled(binding.referenceIndex)) {
            return ExecuteAction(binding.kind, binding.id, ActionContext{g_nppData._nppHandle, editor});
        }
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

void ShowConfigReferenceDialog() {
    const ScopedShortcutHandlingPause pause;
    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_LISTVIEW_CLASSES};
    ::InitCommonControlsEx(&controls);
    ::DialogBoxParamW(ModuleInstance(),
                      MAKEINTRESOURCEW(kConfigDialogId),
                      g_nppData._nppHandle,
                      ConfigReferenceDialogProc,
                      0);
}

void ShowAboutDialog() {
    const ScopedShortcutHandlingPause pause;
    std::wstring summary = L"VSCode Keymap NPP\r\n\r\n";
    summary += L"VSCode Keymap NPP strict mode\r\n\r\n";
    summary += L"- Enforces near 1:1 mappings where Notepad++ equivalents exist\r\n";
    summary += L"- Implements missing line operations (move/duplicate/insert/delete)\r\n";
    summary += L"- Uses a pre-accelerator hook to block conflicting default Notepad++ shortcuts\r\n";
    summary += L"- Unsupported VS Code-only shortcuts are reserved as no-op so they do not trigger non-VS Code Notepad++ actions.\r\n\r\n";
    summary += L"Source bindings: ";
    summary += std::to_wstring(kVsCodeSourceBindingCount);
    summary += L"\r\nMapped: ";
    summary += std::to_wstring(kMappedBindingCount);
    summary += L"\r\nReserved no-op: ";
    summary += std::to_wstring(kReservedNoOpBindingCount);
    summary += L"\r\nDocumented unported: ";
    summary += std::to_wstring(kDocumentedUnportedBindingCount);
    summary += L"\r\n\r\nUse Config/Reference to review and control handled shortcuts.\r\nRepository:\r\nhttps://github.com/14ag/vscode-to-notepadpp\r\n\r\nSettings file:\r\n";
    summary += SettingsFilePathText();
    ::MessageBoxW(g_nppData._nppHandle, summary.c_str(), L"About VSCode Keymap NPP", MB_OK | MB_ICONINFORMATION);
}

void RegisterPluginCommands() {
    std::wmemset(g_funcItems[0]._itemName, 0, menuItemSize);
    std::wmemset(g_funcItems[1]._itemName, 0, menuItemSize);
    ::wcsncpy_s(g_funcItems[0]._itemName, menuItemSize, L"Config/Reference", _TRUNCATE);
    g_funcItems[0]._pFunc = ShowConfigReferenceDialog;
    ::wcsncpy_s(g_funcItems[1]._itemName, menuItemSize, L"About", _TRUNCATE);
    g_funcItems[1]._pFunc = ShowAboutDialog;
}

}  // namespace

extern "C" __declspec(dllexport) void setInfo(NppData notepadPlusData) {
    g_nppData = notepadPlusData;
    LoadSettings();
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
