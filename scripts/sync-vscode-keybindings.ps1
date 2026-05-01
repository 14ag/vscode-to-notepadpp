param(
    [switch]$RefreshSource = $true,
    [string]$SourceUrl = "https://code.visualstudio.com/docs/reference/default-keybindings",
    [string]$SourceHtmlPath,
    [string]$MappingsPath,
    [string]$OutputCoveragePath,
    [string]$OutputBindingsPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot

if (-not $SourceHtmlPath) {
    $SourceHtmlPath = Join-Path $env:TEMP "vscode-default-keybindings.html"
}
if (-not $MappingsPath) {
    $MappingsPath = Join-Path $repoRoot "data\keybinding-mappings.json"
}
if (-not $OutputCoveragePath) {
    $OutputCoveragePath = Join-Path $repoRoot "docs\VSCodeKeybindingCoverage.MD"
}
if (-not $OutputBindingsPath) {
    $OutputBindingsPath = Join-Path $repoRoot "src\GeneratedBindings.inc"
}

function Normalize-HtmlText {
    param([string]$Value)

    $decoded = [System.Net.WebUtility]::HtmlDecode($Value)
    $decoded = ($decoded -replace '<.*?>', '')
    $decoded = ($decoded -replace '\s+', ' ').Trim()
    return $decoded
}

function Get-KeyTokenExpression {
    param([string]$Token)

    $map = @{
        "Backspace" = "VK_BACK"
        "Tab" = "VK_TAB"
        "Enter" = "VK_RETURN"
        "Escape" = "VK_ESCAPE"
        "Space" = "VK_SPACE"
        "PageUp" = "VK_PRIOR"
        "PageDown" = "VK_NEXT"
        "End" = "VK_END"
        "Home" = "VK_HOME"
        "Left" = "VK_LEFT"
        "Up" = "VK_UP"
        "Right" = "VK_RIGHT"
        "Down" = "VK_DOWN"
        "Insert" = "VK_INSERT"
        "Delete" = "VK_DELETE"
        "NumPad0" = "VK_NUMPAD0"
        "," = "VK_OEM_COMMA"
        "." = "VK_OEM_PERIOD"
        "/" = "VK_OEM_2"
        "\" = "VK_OEM_5"
        "[" = "VK_OEM_4"
        "]" = "VK_OEM_6"
        ";" = "VK_OEM_1"
        "'" = "VK_OEM_7"
        '`' = "VK_OEM_3"
        "=" = "VK_OEM_PLUS"
        "-" = "VK_OEM_MINUS"
    }

    if ($map.ContainsKey($Token)) {
        return $map[$Token]
    }

    if ($Token -match '^F([1-9]|1[0-2])$') {
        return "VK_{0}" -f $Token.ToUpperInvariant()
    }

    if ($Token.Length -eq 1) {
        $upper = $Token.ToUpperInvariant()
        if ($upper -match '^[A-Z0-9]$') {
            return "'{0}'" -f $upper
        }
    }

    throw "Unsupported key token '$Token'."
}

function Convert-KeySequenceToCpp {
    param([string]$KeySequence)

    $strokes = @()
    foreach ($strokeText in ($KeySequence -split ' ')) {
        if ([string]::IsNullOrWhiteSpace($strokeText)) {
            continue
        }

        $parts = @($strokeText -split '\+')
        if ($parts.Count -lt 1) {
            throw "Invalid key sequence '$KeySequence'."
        }

        $token = $parts[$parts.Count - 1]
        $mods = @()
        if ($parts.Count -gt 1) {
            $mods = @($parts[0..($parts.Count - 2)])
        }
        $ctrl = $false
        $alt = $false
        $shift = $false

        foreach ($mod in $mods) {
            switch ($mod) {
                "Ctrl" { $ctrl = $true; continue }
                "Alt" { $alt = $true; continue }
                "Shift" { $shift = $true; continue }
                default { throw "Unsupported modifier '$mod' in '$KeySequence'." }
            }
        }

        $strokes += [pscustomobject]@{
            Ctrl = $ctrl
            Alt = $alt
            Shift = $shift
            Vk = (Get-KeyTokenExpression -Token $token)
        }
    }

    if (($strokes.Count -lt 1) -or ($strokes.Count -gt 2)) {
        throw "Only single-stroke and two-stroke bindings are supported for runtime generation: '$KeySequence'."
    }

    return $strokes
}

function Get-ActionSpec {
    param([string]$Handler, [string]$Target)

    switch ($Handler) {
        "nppCommand" {
            if (-not $Target) {
                throw "nppCommand handler requires target."
            }
            return [pscustomobject]@{ Kind = "ActionKind::NppCommand"; Id = $Target }
        }
        "sciCommand" {
            if (-not $Target) {
                throw "sciCommand handler requires target."
            }
            return [pscustomobject]@{ Kind = "ActionKind::SciCommand"; Id = $Target }
        }
        "custom" {
            if (-not $Target) {
                throw "custom handler requires target."
            }
            return [pscustomobject]@{ Kind = "ActionKind::{0}" -f $Target; Id = "0" }
        }
        "reservedNoOp" {
            return [pscustomobject]@{ Kind = "ActionKind::ReservedNoOp"; Id = "0" }
        }
        default {
            throw "Runtime handler '$Handler' is not supported."
        }
    }
}

function Write-GeneratedBindings {
    param(
        [string]$Path,
        [object[]]$Bindings,
        [hashtable]$Counts
    )

    function Format-Stroke {
        param([object]$Stroke)

        return '{{{0}, {1}, {2}, {3}}}' -f `
            $Stroke.Ctrl.ToString().ToLowerInvariant(), `
            $Stroke.Alt.ToString().ToLowerInvariant(), `
            $Stroke.Shift.ToString().ToLowerInvariant(), `
            $Stroke.Vk
    }

    function Format-WideString {
        param([AllowNull()][string]$Value)

        if ([string]::IsNullOrEmpty($Value)) {
            return 'L""'
        }

        $escaped = $Value.Replace('\', '\\').Replace('"', '\"').Replace("`r", '\r').Replace("`n", '\n').Replace("`t", '\t')
        return 'L"{0}"' -f $escaped
    }

    function Format-Bool {
        param([bool]$Value)

        return $Value.ToString().ToLowerInvariant()
    }

    $referenceRows = New-Object System.Collections.Generic.List[string]
    $shortcutRows = New-Object System.Collections.Generic.List[string]
    $chordRows = New-Object System.Collections.Generic.List[string]

    for ($referenceIndex = 0; $referenceIndex -lt $Bindings.Count; $referenceIndex++) {
        $binding = $Bindings[$referenceIndex]
        $referenceRows.Add(
            ('    {{{0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}, {8}, {9}}},' -f `
                (Format-Bool -Value $binding.runtime), `
                (Format-WideString -Value $binding.section), `
                (Format-WideString -Value $binding.subsection), `
                (Format-WideString -Value $binding.label), `
                (Format-WideString -Value $binding.command), `
                (Format-WideString -Value $binding.winKey), `
                (Format-WideString -Value $binding.status), `
                (Format-WideString -Value $binding.handler), `
                (Format-WideString -Value $binding.target), `
                (Format-WideString -Value $binding.notes))
        ) | Out-Null

        if (-not $binding.Runtime) {
            continue
        }

        $strokes = @(Convert-KeySequenceToCpp -KeySequence $binding.winKey)
        $action = Get-ActionSpec -Handler $binding.handler -Target $binding.target

        if ($strokes.Count -eq 1) {
            $stroke = $strokes[0]
            $shortcutRows.Add(
                ('    {{{0}, {1}, {2}, {3}}}, // {4} -> {5}' -f `
                    (Format-Stroke -Stroke $stroke), `
                    $action.Kind, `
                    $action.Id, `
                    $referenceIndex, `
                    $binding.winKey, `
                    $binding.command)
            ) | Out-Null
            continue
        }

        $first = $strokes[0]
        $second = $strokes[1]
        $chordRows.Add(
            ('    {{{0}, {1}, {2}, {3}, {4}}}, // {5} -> {6}' -f `
                (Format-Stroke -Stroke $first), `
                (Format-Stroke -Stroke $second), `
                $action.Kind, `
                $action.Id, `
                $referenceIndex, `
                $binding.winKey, `
                $binding.command)
        ) | Out-Null
    }

    $content = @(
        "// Generated by scripts/sync-vscode-keybindings.ps1. Do not edit by hand."
        ""
        "constexpr size_t kVsCodeSourceBindingCount = $($Counts.total);"
        "constexpr size_t kMappedBindingCount = $($Counts.mapped);"
        "constexpr size_t kReservedNoOpBindingCount = $($Counts.reserved);"
        "constexpr size_t kDocumentedUnportedBindingCount = $($Counts.unported);"
        ""
        "const std::vector<BindingReferenceEntry> kBindingReferences = {"
    )

    if ($referenceRows.Count -gt 0) {
        $content += $referenceRows
    }
    $content += @(
        "};"
        ""
        "const std::vector<ShortcutBinding> kShortcutBindings = {"
    )

    if ($shortcutRows.Count -gt 0) {
        $content += $shortcutRows
    }
    $content += @(
        "};"
        ""
        "const std::vector<ChordBinding> kChordBindings = {"
    )

    if ($chordRows.Count -gt 0) {
        $content += $chordRows
    }
    $content += @(
        "};"
        ""
    )

    $content | Set-Content -Path $Path -Encoding UTF8
}

function Write-CoverageReport {
    param(
        [string]$Path,
        [object[]]$Bindings,
        [hashtable]$Counts
    )

    $lines = New-Object System.Collections.Generic.List[string]
    $lines.Add("# VS Code Windows Keybinding Coverage") | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add("Generated from `https://code.visualstudio.com/docs/reference/default-keybindings`.") | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add("| Metric | Count |") | Out-Null
    $lines.Add("| --- | ---: |") | Out-Null
    $lines.Add("| Total Windows-default bindings | $($Counts.total) |") | Out-Null
    $lines.Add("| Mapped | $($Counts.mapped) |") | Out-Null
    $lines.Add("| Reserved no-op | $($Counts.reserved) |") | Out-Null
    $lines.Add("| Documented unported | $($Counts.unported) |") | Out-Null
    $lines.Add("") | Out-Null

    $currentSection = $null
    $currentSubsection = $null

    foreach ($binding in $Bindings) {
        if ($binding.section -ne $currentSection) {
            $currentSection = $binding.section
            $currentSubsection = $null
            $lines.Add("## $currentSection") | Out-Null
            $lines.Add("") | Out-Null
            $lines.Add("| Key | Command | VS Code command id | Status | Handling | Notes |") | Out-Null
            $lines.Add("| --- | --- | --- | --- | --- | --- |") | Out-Null
        }

        if ($binding.subsection -and ($binding.subsection -ne $currentSubsection)) {
            $currentSubsection = $binding.subsection
            $lines.Add("| | | | | | |") | Out-Null
            $lines.Add("| **$currentSubsection** | | | | | |") | Out-Null
        }

        $notes = $binding.notes
        if (-not $notes) {
            $notes = ""
        }
        $lines.Add("| ``$($binding.winKey)`` | $($binding.label) | ``$($binding.command)`` | $($binding.status) | $($binding.handler) | $notes |") | Out-Null
    }

    $lines.Add("") | Out-Null
    $lines.Add("Status meanings: `mapped` = VS Code shortcut covered, `reserved-noop` = swallowed to avoid bad fallthrough, `documented-unported` = listed but intentionally not emulated.") | Out-Null
    $lines | Set-Content -Path $Path -Encoding UTF8
}

function Get-StatusFromHandler {
    param([string]$Handler)

    switch ($Handler) {
        "nppCommand" { return "mapped" }
        "sciCommand" { return "mapped" }
        "custom" { return "mapped" }
        "nativePassThrough" { return "mapped" }
        "reservedNoOp" { return "reserved-noop" }
        "documentedUnported" { return "documented-unported" }
        default { throw "Unknown handler '$Handler'." }
    }
}

function Get-RuntimeFlag {
    param([string]$Handler)

    return @("nppCommand", "sciCommand", "custom", "reservedNoOp") -contains $Handler
}

function Get-DefaultBindingRecord {
    param([object]$Binding)

    return [ordered]@{
        section = $Binding.section
        subsection = $Binding.subsection
        label = $Binding.label
        command = $Binding.command
        winKey = $Binding.winKey
        status = "documented-unported"
        handler = "documentedUnported"
        target = $null
        notes = ""
        runtime = $false
    }
}

function Get-ManifestOnlyBindingRecord {
    param([object]$Mapping)

    $requiredFields = @("section", "label")
    foreach ($field in $requiredFields) {
        if (-not ($Mapping.PSObject.Properties.Name -contains $field) -or [string]::IsNullOrWhiteSpace($Mapping.$field)) {
            throw "Manifest-only binding requires '$field': $($Mapping.command) / $($Mapping.winKey)"
        }
    }

    $target = $null
    $notes = ""
    $subsection = ""
    if ($Mapping.PSObject.Properties.Name -contains "target") {
        $target = $Mapping.target
    }
    if ($Mapping.PSObject.Properties.Name -contains "notes") {
        $notes = $Mapping.notes
    }
    if ($Mapping.PSObject.Properties.Name -contains "subsection") {
        $subsection = $Mapping.subsection
    }

    return [ordered]@{
        section = $Mapping.section
        subsection = $subsection
        label = $Mapping.label
        command = $Mapping.command
        winKey = $Mapping.winKey
        status = Get-StatusFromHandler -Handler $Mapping.handler
        handler = $Mapping.handler
        target = $target
        notes = $notes
        runtime = Get-RuntimeFlag -Handler $Mapping.handler
    }
}

if ($RefreshSource -or (-not (Test-Path -LiteralPath $SourceHtmlPath))) {
    New-Item -ItemType Directory -Path (Split-Path -Parent $SourceHtmlPath) -Force | Out-Null
    curl.exe --silent --show-error -L $SourceUrl -o $SourceHtmlPath | Out-Null
}

if (-not (Test-Path -LiteralPath $SourceHtmlPath)) {
    throw "VS Code keybinding source HTML not found: $SourceHtmlPath"
}
if (-not (Test-Path -LiteralPath $MappingsPath)) {
    throw "Keybinding mapping manifest not found: $MappingsPath"
}

$html = Get-Content -Raw $SourceHtmlPath
$htmlDateMatch = [regex]::Match($html, '<meta name="ms\.date" content="(?<date>[^"]+)"')
$sourceDate = if ($htmlDateMatch.Success) { $htmlDateMatch.Groups["date"].Value } else { "" }

$headingPattern = '<h(?<level>[23])[^>]*>(?<title>.*?)</h[23]>'
$rowPattern = '<tr>\s*<td>(?<label>.*?)</td>\s*<td><span class="dynamic-keybinding"(?<attrs>.*?)data-commandId="(?<dataCommand>.*?)"(?<attrs2>.*?)data-win="(?<win>.*?)"(?<attrs3>.*?)>.*?</span></td>\s*<td><code>(?<command>.*?)</code></td>'

$headingMatches = [regex]::Matches($html, $headingPattern, [System.Text.RegularExpressions.RegexOptions]::Singleline)
$rowMatches = [regex]::Matches($html, $rowPattern, [System.Text.RegularExpressions.RegexOptions]::Singleline)

$parsedRows = New-Object System.Collections.Generic.List[object]
$headingIndex = 0
$currentSection = ""
$currentSubsection = ""

foreach ($row in $rowMatches) {
    while (($headingIndex -lt $headingMatches.Count) -and ($headingMatches[$headingIndex].Index -lt $row.Index)) {
        $heading = $headingMatches[$headingIndex]
        $headingTitle = Normalize-HtmlText -Value $heading.Groups["title"].Value
        if ($heading.Groups["level"].Value -eq "2") {
            $currentSection = $headingTitle
            $currentSubsection = ""
        }
        else {
            $currentSubsection = $headingTitle
        }
        $headingIndex++
    }

    $winKey = (Normalize-HtmlText -Value $row.Groups["win"].Value)
    if ([string]::IsNullOrWhiteSpace($winKey)) {
        continue
    }

    $command = (Normalize-HtmlText -Value $row.Groups["command"].Value)
    $label = (Normalize-HtmlText -Value $row.Groups["label"].Value)

    $parsedRows.Add([pscustomobject]@{
        section = $currentSection
        subsection = $currentSubsection
        label = $label
        command = $command
        winKey = $winKey
    }) | Out-Null
}

$dedupedRows = New-Object System.Collections.Generic.List[object]
$seenSourceBindings = @{}
foreach ($row in $parsedRows) {
    $key = "$($row.command)`n$($row.winKey)"
    if ($seenSourceBindings.ContainsKey($key)) {
        continue
    }
    $seenSourceBindings[$key] = $true
    $dedupedRows.Add($row) | Out-Null
}

$mappingDoc = Get-Content -Raw $MappingsPath | ConvertFrom-Json
$mappingLookup = @{}
$mappingOrder = New-Object System.Collections.Generic.List[string]
foreach ($item in $mappingDoc.bindings) {
    $key = "$($item.command)`n$($item.winKey)"
    if ($mappingLookup.ContainsKey($key)) {
        throw "Duplicate mapping manifest entry for $($item.command) / $($item.winKey)."
    }
    $mappingLookup[$key] = $item
    $mappingOrder.Add($key) | Out-Null
}

$resolvedBindings = New-Object System.Collections.Generic.List[object]
foreach ($binding in $dedupedRows) {
    $record = Get-DefaultBindingRecord -Binding $binding
    $key = "$($binding.command)`n$($binding.winKey)"
    if ($mappingLookup.ContainsKey($key)) {
        $mapping = $mappingLookup[$key]
        $target = $null
        $notes = ""
        if ($mapping.PSObject.Properties.Name -contains "target") {
            $target = $mapping.target
        }
        if ($mapping.PSObject.Properties.Name -contains "notes") {
            $notes = $mapping.notes
        }
        $record.handler = $mapping.handler
        $record.status = Get-StatusFromHandler -Handler $mapping.handler
        $record.target = $target
        $record.notes = $notes
        $record.runtime = Get-RuntimeFlag -Handler $mapping.handler
    }
    $resolvedBindings.Add([pscustomobject]$record) | Out-Null
}

foreach ($lookupKey in $mappingOrder) {
    $match = $resolvedBindings | Where-Object { "$($_.command)`n$($_.winKey)" -eq $lookupKey }
    if (-not $match) {
        $record = [pscustomobject](Get-ManifestOnlyBindingRecord -Mapping $mappingLookup[$lookupKey])
        $insertIndex = -1
        for ($index = $resolvedBindings.Count - 1; $index -ge 0; $index--) {
            if (($resolvedBindings[$index].section -eq $record.section) -and
                ($resolvedBindings[$index].subsection -eq $record.subsection)) {
                $insertIndex = $index + 1
                break
            }
        }

        if ($insertIndex -ge 0) {
            $resolvedBindings.Insert($insertIndex, $record)
        }
        else {
            $resolvedBindings.Add($record) | Out-Null
        }
    }
}

$runtimeKeys = @{}
foreach ($binding in $resolvedBindings) {
    if (-not $binding.runtime) {
        continue
    }

    $normalized = @(Convert-KeySequenceToCpp -KeySequence $binding.winKey)
    if ($normalized.Count -eq 1) {
        $id = "{0}|{1}|{2}|{3}" -f $normalized[0].Ctrl, $normalized[0].Alt, $normalized[0].Shift, $normalized[0].Vk
        if ($runtimeKeys.ContainsKey($id)) {
            throw "Duplicate runtime shortcut collision: $($binding.winKey) conflicts with $($runtimeKeys[$id])."
        }
        $runtimeKeys[$id] = $binding.command
        continue
    }

    $id = "{0}|{1}|{2}|{3}::{4}|{5}|{6}|{7}" -f `
        $normalized[0].Ctrl, $normalized[0].Alt, $normalized[0].Shift, $normalized[0].Vk, `
        $normalized[1].Ctrl, $normalized[1].Alt, $normalized[1].Shift, $normalized[1].Vk
    if ($runtimeKeys.ContainsKey($id)) {
        throw "Duplicate runtime chord collision: $($binding.winKey) conflicts with $($runtimeKeys[$id])."
    }
    $runtimeKeys[$id] = $binding.command
}

$counts = @{
    total = $resolvedBindings.Count
    mapped = @($resolvedBindings | Where-Object { $_.status -eq "mapped" }).Count
    reserved = @($resolvedBindings | Where-Object { $_.status -eq "reserved-noop" }).Count
    unported = @($resolvedBindings | Where-Object { $_.status -eq "documented-unported" }).Count
}

New-Item -ItemType Directory -Path (Split-Path -Parent $OutputCoveragePath) -Force | Out-Null
New-Item -ItemType Directory -Path (Split-Path -Parent $OutputBindingsPath) -Force | Out-Null

Write-CoverageReport -Path $OutputCoveragePath -Bindings $resolvedBindings -Counts $counts
Write-GeneratedBindings -Path $OutputBindingsPath -Bindings $resolvedBindings -Counts $counts

Write-Output "[OK] Parsed $($counts.total) Windows-default VS Code bindings."
Write-Output "[OK] Mapped: $($counts.mapped); reserved: $($counts.reserved); documented-unported: $($counts.unported)."
Write-Output "[OK] Wrote $OutputCoveragePath"
Write-Output "[OK] Wrote $OutputBindingsPath"
