# Host reproductions for OPL parser bugs on malformed input (Windows, MSVC with AddressSanitizer).
#
#   powershell -ExecutionPolicy Bypass -File labs\host-repro\run_all.ps1 [-Opl <OPL checkout>]
#
# Each harness copies the functions under test out of the OPL checkout (extract.py), so the same
# run works on master and on a fix branch. Every test is built twice: plain (/GS-, so a small stack
# overrun does not abort) for the output, and with /fsanitize=address for memory errors.
# -Images <dir> also prints the game ID OPL derives from every .iso image in <dir>.
# -Only cheat,iso,find,zso,cnf,sb,name,crc,cfg,... runs only those groups.
# -Expected <file> compares every result line with <file> (the output of an earlier run, without the
# "OPL tree:" line) and exits with 1 on any difference. CI runs the checks this way.
param([string]$Opl = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path, [string]$Images = "", [string[]]$Only = @(), [string]$Expected = "")
$ErrorActionPreference = "Stop"
$here = $PSScriptRoot
$build = Join-Path $here "build"
$inputs = Join-Path $build "in"
New-Item -ItemType Directory -Force $build | Out-Null

$vs = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath
$vcvars = Join-Path $vs "VC\Auxiliary\Build\vcvars64.bat"
# The ASan runtime must come from the toolset vcvars64.bat selects: a runner with several toolsets has
# several copies, and a binary built by a newer compiler does not start with an older DLL (0xC0000139)
$vcTools = (cmd /c "`"$vcvars`" >nul 2>nul && echo %VCToolsInstallDir%" | Select-Object -Last 1).Trim()
$asanDir = if ($vcTools -and (Test-Path (Join-Path $vcTools "bin\Hostx64\x64\clang_rt.asan_dynamic-x86_64.dll"))) {
    Join-Path $vcTools "bin\Hostx64\x64"
} else {
    (Get-ChildItem (Join-Path $vs "VC\Tools\MSVC") -Recurse -Filter "clang_rt.asan_dynamic-x86_64.dll" |
        Where-Object { $_.FullName -like "*Hostx64\x64*" } | Sort-Object FullName | Select-Object -Last 1).DirectoryName
}
$env:PATH = "$asanDir;$env:PATH"
python -I (Join-Path $here "make_inputs.py") $inputs | Out-Null

$Only = @($Only | ForEach-Object { $_ -split "," } | Where-Object { $_ })
function Want([string]$Group) { return ($Only.Count -eq 0) -or ($Only -contains $Group) }

$script:lines = @()
function Emit([string]$Line) {
    $script:lines += $Line
    $Line
}

function Build([string]$Harness, [string]$Tag, [string]$Source, [string[]]$Funcs, [string]$Source2 = "", [string[]]$Funcs2 = @(), [string]$Source3 = "", [string[]]$Funcs3 = @(), [string]$Source4 = "", [string[]]$Funcs4 = @(), [string]$Source5 = "", [string[]]$Funcs5 = @(), [string[]]$Patch = @(), [string]$Defines = "") {
    $text = ""
    if ($Source5) { $text += (python -I (Join-Path $here "extract.py") (Join-Path $Opl $Source5) @Funcs5 | Out-String) }
    if ($Source4) { $text += (python -I (Join-Path $here "extract.py") (Join-Path $Opl $Source4) @Funcs4 | Out-String) }
    if ($Source3) { $text += (python -I (Join-Path $here "extract.py") (Join-Path $Opl $Source3) @Funcs3 | Out-String) }
    if ($Source2) { $text += (python -I (Join-Path $here "extract.py") (Join-Path $Opl $Source2) @Funcs2 | Out-String) }
    $text += (python -I (Join-Path $here "extract.py") (Join-Path $Opl $Source) @Funcs | Out-String)
    foreach ($p in $Patch) {
        $a, $b = $p -split "=>", 2
        $text = $text.Replace($a, $b)
    }
    Set-Content -Encoding ascii (Join-Path $build ($Harness + "_extracted.c")) $text
    $src = Join-Path $here ($Harness + "_harness.c")
    $exe = Join-Path $build "$Harness-$Tag"
    $common = "/nologo /W3 /Od /Zi /wd4996 $Defines /I`"$build`" `"$src`""
    cmd /c "`"$vcvars`" >nul 2>nul && cl $common /GS- /Fe:`"$exe.exe`" /Fo:`"$exe.obj`" /Fd:`"$exe.pdb`" >`"$exe.log`" && cl $common /fsanitize=address /Fe:`"$exe-asan.exe`" /Fo:`"$exe-asan.obj`" /Fd:`"$exe-asan.pdb`" >>`"$exe.log`""
    if ($LASTEXITCODE -ne 0) { Get-Content "$exe.log"; throw "build of $Harness failed" }
}

function Run([string]$Exe, [string[]]$Arguments) {
    # Process rather than Start-Process: the latter can lose the exit code of a process that ends at once
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = Join-Path $build "$Exe.exe"
    $psi.Arguments = (@($Arguments | ForEach-Object { if ($_ -match "\s") { "`"$_`"" } else { $_ } })) -join " "
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $p = [System.Diagnostics.Process]::Start($psi)
    $stdout = $p.StandardOutput.ReadToEndAsync()
    $stderr = $p.StandardError.ReadToEndAsync()
    if (-not $p.WaitForExit(10000)) { $p.Kill(); return "TIMEOUT: endless loop" }
    $p.WaitForExit()
    $lines = @($stdout.Result -split "`r?`n" | Where-Object { $_ } | Select-Object -Last 3)
    $err = $stderr.Result -split "`r?`n"
    $asan = $err | Select-String -Pattern "ERROR: AddressSanitizer: ([\w-]+)" | Select-Object -First 1
    $frame = $err | Select-String -Pattern "#\d+ 0x\w+ in (\w+) .*(extracted|harness)" | Select-Object -First 1
    if ($asan) { return "ASAN " + $asan.Matches[0].Groups[1].Value + " in " + $frame.Matches[0].Groups[1].Value }
    if ($p.ExitCode -ne 0) { return ("CRASH 0x{0:X8}" -f $p.ExitCode) }
    return ($lines -join " | ")
}

function Test([string]$Name, [string]$Exe, [string[]]$Arguments) {
    Emit ("{0,-34} plain: {1}" -f $Name, (Run $Exe $Arguments))
    Emit ("{0,-34} asan:  {1}" -f "", (Run "$Exe-asan" $Arguments))
}

"OPL tree: $Opl"
if (Want "cheat") {
    $cheatFuncs = "make_code", "is_cheat_code", "parse_line", "is_cmt_str", "chr_idx", "term_str", "is_empty_str", "trim_str", "is_empty_substr", "parse_buf", "CHEAT_LINE_MAX", "set_cheats_list"
    Build cheat t "src\cheatman.c" $cheatFuncs -Source2 "ee_core\src\cheat_api.c" -Funcs2 "SetupCheats"
    foreach ($t in "cht_plain", "cht_indented", "cht_251", "cht_long_last", "cht_no_lf", "cht_utf8") { Test "cheat $t" cheat-t (Join-Path $inputs "$t.cht") }
    # every cheat enabled, then the list for the EE core and the engine's hook and code tables
    foreach ($t in "cht_addr0", "cht_9hooks", "cht_many") { Test "cheat engine $t" cheat-t "-e", (Join-Path $inputs "$t.cht") }
    if ((Get-Content -Raw (Join-Path $Opl "src\cheatman.c")).Contains("char digits[CODE_DIGITS];")) {
        Emit "(make_code() overruns digits[] on every code; again with only that line fixed, to see the other bugs)"
        Build cheat iso "src\cheatman.c" $cheatFuncs -Source2 "ee_core\src\cheat_api.c" -Funcs2 "SetupCheats" -Patch "char digits[CODE_DIGITS];=>char digits[CODE_DIGITS + 1];"
        foreach ($t in "cht_251", "cht_long_last", "cht_no_lf") { Test "cheat $t (digits fixed)" cheat-iso (Join-Path $inputs "$t.cht") }
        foreach ($t in "cht_addr0", "cht_9hooks", "cht_many") { Test "cheat engine $t (digits fixed)" cheat-iso "-e", (Join-Path $inputs "$t.cht") }
    }
}

if (Want "iso") {
    Build iso ee "modules\isofs\isofs.c" "dirTocEntry", "cdvdman_locatefile"
    Build iso ingame "modules\iopcore\cdvdman\searchfile.c" "cdvdman_locatefile" -Source2 "modules\iopcore\cdvdman\internal.h" -Funcs2 "dirTocEntry" -Defines "/DINGAME"
    foreach ($t in "ok", "zero", "xa", "long") {
        Test "isofs locatefile $t" iso-ee $t
        Test "cdvdman locatefile $t" iso-ingame $t
    }
}

if (Want "find") {
    Build find t "modules\isofs\isofs.c" "dirTocEntry", "cdvdman_trimspaces", "cdvdman_locatefile", "cdvdman_findfile"
    foreach ($n in "\SYSTEM.CNF;1", "/SYSTEM.CNF;1", "\A_VERY_LONG_LEVEL2_FILENAME.TXT;1") { Test "isofs findfile $n" find-t $n }
}

if (Want "zso") {
    $zsoDefine = if ((Get-Content -Raw (Join-Path $Opl "modules\isofs\zso.c")).Contains("int ziso_init(")) { "/DINIT_RETURNS_INT" } else { "" }
    # the LZ4 decoder of the tree, and 32-bit pointer arithmetic kept whole on a 64-bit host
    Copy-Item (Join-Path $Opl "modules\isofs\lz4.c"), (Join-Path $Opl "modules\isofs\lz4.h") $build
    $zsoPatch = "(u32)ziso_tmp_buf & 63=>(size_t)ziso_tmp_buf & 63", "(void *)(((u32)ziso_tmp_buf & (~63)) + 64)=>(void *)(((size_t)ziso_tmp_buf & (~63)) + 64)"
    Build zso t "modules\isofs\isofs.c" "longLseek", "read_raw_data" -Source2 "modules\isofs\zso.c" -Funcs2 "ziso_init", "?ziso_read_slice", "ziso_read_sector" -Defines $zsoDefine -Patch $zsoPatch
    foreach ($a in "0", "11", "12", "255") { Test "zso align $a" zso-t (Join-Path $inputs "zero.bin"), $a }
    # sector reads over LZ4 and raw blocks into buffers of exactly count * 2048 bytes
    foreach ($c in "ok 0 4", "ok 7 3", "align4 0 40", "ok 100 300", "badidx 3 1", "16k 0 4") {
        $a = $c.Split(" ")
        Test "zso read $c" zso-t "read", (Join-Path $inputs "zso_$($a[0]).zso"), $a[1], $a[2]
    }
}

if (Want "cnf") {
    Build cnf t "src\ps2cnf.c" "CNF_LEN_MAX", "CNFGetToken", "CNFAdvanceLine", "CNFGetKey", "ps2cnfGetBootFile" -Patch 'fopen(path, "r")=>fopen(path, "rb")'
    foreach ($t in "cnf_ok", "cnf_no_eq") { Test "ps2cnf $t" cnf-t (Join-Path $inputs "$t.cnf") }
}

if (Want "sb") {
    $sbSource = Get-Content -Raw (Join-Path $Opl "src\supportbase.c")
    $sbRoDefine = if ($sbSource.Contains("sbListCacheReadOnly")) { " /DHAVE_SB_RO" } elseif ($sbSource.Contains("bdmIsReadOnlyPath(path)")) { " /DHAVE_SB_RO_PATH" } else { "" }
    Build sb t "src\supportbase.c" "game_list_t", "game_cache_list", "sbReadList", "updateISOGameList" -Defines ("/DHAVE_SBREADLIST /DHAVE_UPDATE" + $sbRoDefine)
    foreach ($c in "ul64 0", "ul65 0", "ul129 0", "ul65 2", "ul129 2") {
        $a = $c.Split(" ")
        Test "sbReadList $c" sb-t "ul", (Join-Path $inputs $a[0]), $a[1]
    }
    foreach ($c in "2 2", "3 3 swap", "1 2") { Test "updateISOGameList $c" sb-t (@("cache", (Join-Path $inputs "cachedir")) + $c.Split(" ")) }
    # A read-only UDPBD server: the changed list must not be written to the device
    Test "updateISOGameList 1 2 read-only" sb-t (@("cachero", (Join-Path $inputs "cachedir")) + "1 2".Split(" "))
}

if (Want "name") {
    Build name t "src\supportbase.c" "isValidIsoName"
    Test "isValidIsoName" name-t "..", "x.iso", "SLUS_200.62.Title.iso"
}

if (Want "crc") {
    Build crc t "src\system.c" "USBA_crc32"
    $names = '""', "41", "4772616e64205468656674204175746f20494949", "c8e3f0e0", "d098d0b3d180d0b0"
    Test "USBA_crc32" crc-t $names
    Emit ("{0,-34} ref:   {1}" -f "", ((python -I (Join-Path $here "usba_ref.py") @names) -join " | "))
}

if (Want "cfg") {
    # void * arithmetic in CheckPS2Logo() is a GCC extension
    $cfgDefine = if ((Get-Content -Raw (Join-Path $Opl "src\config.c")).Contains("size_t prefixmax")) { "/DPREFIX_MAX" } else { "" }
    Build cfg t "src\config.c" "strToColor", "isWS", "splitAssignment", "parsePrefix", "configReadLegacyIP" -Source2 "src\util.c" -Funcs2 "openFileBuffer", "readFileBuffer", "closeFileBuffer", "min", "fromHex", "CheckPS2Logo" -Patch "void *buffer = logo;=>u8 *buffer = logo;" -Defines $cfgDefine
    Test "readFileBuffer plain cfg" cfg-t "lines", (Join-Path $inputs "cfg_plain.cfg")
    Test "readFileBuffer '#' last line, no LF" cfg-t "lines", (Join-Path $inputs "cfg_comment_end.cfg")
    Test "readFileBuffer read() fails" cfg-t "lines", (Join-Path $inputs "cfg_plain.cfg"), "readerror"
    Test "strToColor #112233" cfg-t "color", "#112233"
    Test "strToColor #11223344556677" cfg-t "color", "#11223344556677"
    Test "splitAssignment 40-char key" cfg-t "split", ("k" * 40 + "=value")
    Test "parsePrefix 60-char prefix" cfg-t "prefix", ("A" * 60 + ": no equals sign")
    Test "configReadLegacyIP 300 bytes" cfg-t "legacyip", (Join-Path $inputs "IPCONFIG.DAT")
    Test "CheckPS2Logo HDD" cfg-t "logo"
}

if (Want "cheatsel") {
    Build cheatsel t "src\cheatconfig.c" "normalizeCheatName", "cheatConfigIsMasterCode", "cheatNamesMatch", "cheatConfigLoadSelections", "cheatConfigSaveSelections"
    Test "cheat picks, distinct names" cheatsel-t "+Mastercode", "+Infinite Health", "Max Money", "+Moon Jump"
    Test "cheat picks, duplicate names" cheatsel-t "+Mastercode", "+Infinite Health", "+Infinite Health", "Max Money"
}

if (Want "cfgwrite") {
    Build cfgwrite t "src\config.c" "configWrite" -Source2 "src\util.c" -Funcs2 "openFileBuffer", "writeFileBuffer", "closeFileBuffer"
    foreach ($m in "ok", "fail", "short") { Test "configWrite, write() $m" cfgwrite-t (Join-Path $build "cfgwrite-$m.cfg"), $m }
}

if (Want "cdread") {
    # The IOP has 32-bit pointers; keep them whole on a 64-bit host
    Build cdread t "modules\iopcore\cdvdman\cdvdman.c" "cdvdman_read" -Patch "(void *)((u32)buf + offset)=>(void *)((u8 *)buf + offset)", "(u32)(buf)&3=>(size_t)(buf)&3"
    foreach ($c in "2048 2", "2340 1", "2340 3", "2328 2") { Test "cdvdman_read $c" cdread-t $c.Split(" ") }
}

if (Want "cfgsave") {
    # Saving the settings when one file of them cannot be written where they were read
    Build cfgsave t "src\config.c" "configMove", "configSetMove", "configWrite", "configWriteMulti" -Source2 "src\util.c" -Funcs2 "openFileBuffer", "writeFileBuffer", "closeFileBuffer"
    foreach ($v in "plain", "asan") {
        $dir = Join-Path $build "cfgsave"
        if (Test-Path $dir) { Remove-Item -Recurse -Force $dir }
        New-Item -ItemType Directory -Force (Join-Path $dir "home\conf_opl.cfg"), (Join-Path $dir "other") | Out-Null
        $exe = if ($v -eq "plain") { "cfgsave-t" } else { "cfgsave-t-asan" }
        Emit ("{0,-34} {1}: {2}" -f $(if ($v -eq "plain") { "Save, conf_opl.cfg fails" } else { "" }), $v.PadRight(5), (Run $exe @((Join-Path $dir "home"), (Join-Path $dir "other"))))
    }
}

if (Want "cfgloc") {
    # Where the settings are read and saved, by the device OPL was started from
    $cfglocDefine = ""
    if ((Get-Content -Raw (Join-Path $Opl "src\opl.c")).Contains("tryBootDevice(")) { $cfglocDefine += " /DHAVE_BOOT_FIRST" }
    if ((Get-Content -Raw (Join-Path $Opl "src\config.c")).Contains("configGetDeviceRank(")) { $cfglocDefine += " /DHAVE_RANK" }
    $cfglocFuncs = "checkLoadConfigBDM", "checkLoadConfigHDD", "?checkLoadConfigMMCE", "?BOOT_DEVICE_OTHER", "?BOOT_DEVICE_BDM", "?BOOT_DEVICE_HDD", "?BOOT_DEVICE_MMCE0", "?BOOT_DEVICE_MMCE1", "?getBootDevice", "?tryBootDevice", "?trySetBootHome", "tryAlternateDevice", "trySaveConfigBDM", "trySaveConfigHDD", "trySaveConfigMC", "?trySaveConfigMMCE", "trySaveAlternateDevice"
    Build cfgloc t "src\opl.c" $cfglocFuncs -Source2 "src\bdmsupport.c" -Funcs2 "?bdmGetDriverName", "?bdmCanHoldSettings", "?bdmFindSettingsDevice", "bdmFindPartition" -Source3 "src\config.c" -Funcs3 "?configGetDeviceRank" -Defines $cfglocDefine
    foreach ($s in "usb", "usb-root", "usb-first", "usb-apa", "mc", "hdd", "hdd-pfs", "mmce", "mmce-first", "usb-prefix", "mx4sio", "udpbd", "usb-mass1") { Test "Settings $s" cfgloc-t $s }
}

if (Want "bdmprobe") {
    Build bdmprobe t "src\bdmsupport.c" "?bdmGetDriverName", "?bdmCanHoldSettings", "bdmFindPartition"
    $dir = Join-Path $build "bdmprobe"
    foreach ($v in "plain", "asan") {
        New-Item -ItemType Directory -Force (Join-Path $dir "mass0") | Out-Null
        Set-Content -Encoding ascii (Join-Path $dir "mass0\conf_opl.cfg") "theme=Some theme`r`nlanguage=Russian`r`n"
        $exe = if ($v -eq "plain") { "bdmprobe-t" } else { "bdmprobe-t-asan" }
        Emit ("{0,-34} {1}: {2}" -f $(if ($v -eq "plain") { "bdmFindPartition write probe" } else { "" }), $v.PadRight(5), (Run $exe $dir))
    }
    # mass0 a read-only UDPBD server, mass1 a USB drive: OPL's settings must go to mass1
    New-Item -ItemType Directory -Force (Join-Path $dir "mass1") | Out-Null
    foreach ($v in "plain", "asan") {
        $exe = if ($v -eq "plain") { "bdmprobe-t" } else { "bdmprobe-t-asan" }
        Emit ("{0,-34} {1}: {2}" -f $(if ($v -eq "plain") { "bdmFindPartition read-only mass0" } else { "" }), $v.PadRight(5), (Run $exe @($dir, "ro0")))
    }
    # no settings on mass0 yet: the probe must not leave an empty conf_opl.cfg, read at the next start if the save fails
    foreach ($v in "plain", "asan") {
        Remove-Item -Force -ErrorAction SilentlyContinue (Join-Path $dir "mass0\conf_opl.cfg")
        $exe = if ($v -eq "plain") { "bdmprobe-t" } else { "bdmprobe-t-asan" }
        Emit ("{0,-34} {1}: {2}" -f $(if ($v -eq "plain") { "bdmFindPartition probe, no file yet" } else { "" }), $v.PadRight(5), (Run $exe $dir))
    }
}

if (Want "vmc") {
    Build vmc t "src\system.c" "sysCheckVMC"
    $dir = Join-Path $build "vmc"
    New-Item -ItemType Directory -Force (Join-Path $dir "VMC") | Out-Null
    # card size in bytes (-1: no card), createSize as the VMC menu passes it
    foreach ($c in @(@(4194304, 8, "4 MiB card, create 8 MB"), @(8650752, 8, "PCSX2 .ps2 size, create 8 MB"), @(8388608, 8, "8 MiB card, create 8 MB"), @(-1, 8, "no card, create 8 MB"), @(4194304, -1, "4 MiB card, delete"))) {
        foreach ($v in "plain", "asan") {
            $card = Join-Path $dir "VMC\keep.bin"
            Remove-Item $card -ErrorAction SilentlyContinue
            if ($c[0] -ge 0) { [IO.File]::WriteAllBytes($card, (New-Object byte[] $c[0])) }
            $exe = if ($v -eq "plain") { "vmc-t" } else { "vmc-t-asan" }
            Emit ("{0,-34} {1}: {2}" -f $(if ($v -eq "plain") { "sysCheckVMC " + $c[2] } else { "" }), $v.PadRight(5), (Run $exe $dir, "keep", "$($c[1])"))
        }
    }
}

if (Want "text") {
    $msgDefine = if ((Get-Content -Raw (Join-Path $Opl "src\gui.c")).Contains("guiDrawMsgText(")) { "/DHAVE_MSGTEXT" } else { "" }
    Build text t "src\themes.c" "drawAttributeText" -Source2 "src\fntsys.c" -Funcs2 "fntFitString" -Source3 "src\gui.c" -Funcs3 "?guiDrawMsgText" -Defines $msgDefine
    Test "fntFitString fits, exact buffer" text-t "fit", "200", "two words"
    Test "fntFitString long word, exact buffer" text-t "fit", "40", "Averyveryverylongword"
    Test "fntFitString 255-char config value" text-t "cfgval", "100"
    Test "AttributeText wrap, config value" text-t "attr", "100", "The quick brown fox jumps over the lazy dog"
    Test "message box, short" text-t "msgbox", "Please wait..."
    Test "message box, read-only VMC note" text-t "msgbox", "This UDPBD server is read-only, so no VMC can be used on it; the game saves to the real memory card."
    foreach ($t in "ru", "ar", "long") { Test "message box, $t" text-t "msgboxfile", (Join-Path $inputs "msg_$t.txt") }
}

if (Want "theme") {
    Build theme t "src\themes.c" "initMutableText", "getGameCount", "drawGameCountText", "initGameCountText", "freeImageTexture", "initImageTexture", "initImageInternalTexture", "validateItemsList", "validateGUIElems"
    Test "GameCountText count buffer" theme-t "gamecount"
    Test "image file missing" theme-t "image"
    Test "internal image unknown" theme-t "internal"
    Test "theme without ItemsList" theme-t "itemslist"
}

if (Want "thmcolors") {
    Build thmcolors t "src\themes.c" "?thmSetTextColor", "thmSetColors"
    Test "Theme colours, internal theme" thmcolors-t "colors"
}

if (Want "bdmindex") {
    Build bdmindex t "src\themes.c" "drawBDMIndex"
    foreach ($s in "one", "two", "gap", "slot2", "three", "four", "five", "late") { Test "BdmIndex $s" bdmindex-t $s }
    Test "BdmIndex gap, own 5-dot pictures" bdmindex-t "gap", "fixed"
}

if (Want "lang") {
    Build lang t "src\lang.c" "lngReadEntry"
    Test "lngReadEntry file names" lang-t "lang_Russian.lng", "abcd.lng", "ru.lng"
}

if (Want "thmlist") {
    # The same theme or language on several devices, and a device that goes away
    $removeDefine = if ((Get-Content -Raw (Join-Path $Opl "src\themes.c")).Contains("thmRemoveElements(")) { "/DHAVE_REMOVE" } else { "" }
    $thmFuncs = "thmReadEntry", "thmRebuildGuiNames", "thmAddElements", "?thmReinit", "?thmRemoveElements", "thmSetGuiValue", "thmGetGuiValue", "thmFindGuiID", "thmGetFilePath", "thmGetGuiList"
    $lngFuncs = "lngReadEntry", "lngRebuildLangNames", "lngAddLanguages", "?lngRemoveLanguages", "lngSetGuiValue", "lngGetGuiValue", "lngFindGuiID", "lngGetFilePath", "lngGetGuiList"
    # A device whose prefix changes: the tree's helper, and whether the BDM and MMCE code call it then
    if ((Get-Content -Raw (Join-Path $Opl "src\supportbase.c")).Contains("void sbDropThemesLanguages(")) { $removeDefine += " /DHAVE_SB_DROP" }
    if ((Get-Content -Raw (Join-Path $Opl "src\bdmsupport.c")).Contains("sbDropThemesLanguages(oldPrefix")) { $removeDefine += " /DHAVE_BDM_PREFIX_DROP" }
    $mmce = Join-Path $Opl "src\mmcesupport.c"
    if ((Test-Path $mmce) -and (Get-Content -Raw $mmce).Contains("sbDropThemesLanguages(oldPrefix")) { $removeDefine += " /DHAVE_MMCE_PREFIX_DROP" }
    Build thmlist t "src\themes.c" $thmFuncs -Source2 "src\lang.c" -Funcs2 $lngFuncs -Source3 "src\config.c" -Funcs3 "?configGetDeviceRank" -Source4 "src\supportbase.c" -Funcs4 "?sbDropThemesLanguages" -Source5 "src\util.c" -Funcs5 "?listDeviceFiles", "?dropDeviceFiles" -Defines $removeDefine
    foreach ($s in "dup-mc", "dup-usb", "remove", "fallback", "shift", "smb", "prefix-bdm", "prefix-mmce", "dialog") { Test "Theme/language list $s" thmlist-t $s }
}

if (Want "history") {
    Build history t "src\OSDHistory.c" "GetBootFilename", "AddHistoryRecordUsingFullPath"
    Test "GetBootFilename" history-t "cdrom0:\SLUS_200.62;1", "cdrom0:\DATA\ABCDEFGHIJKLMNOPQRST.ELF;1"
}

if (Want "gsm") {
    Build gsm t "src\gsm.c" "PrepareGSM" -Source2 "include\pggsm.h" -Funcs2 "makeDISPLAY", "makeSYNCV", "predef_vmode_struct" -Source3 "ee_core\include\coreconfig.h" -Funcs3 "GsmConfig_t"
    foreach ($m in "0", "28", "29", "-1") { Test "PrepareGSM `$GSMVMode=$m" gsm-t $m }
}

if (Want "pad") {
    Build pad t "src\pad.c" "CLOCKS_PER_MILISEC", "getKeyDelay", "readPads"
    Test "readPads tick counter wrap" pad-t "wrap"
}

if (Want "genvmc") {
    Build genvmc t "modules\vmc\genvmc\genvmc.c" "mc_writecluster"
    Test "genvmc write ok" genvmc-t "0"
    Test "genvmc write short" genvmc-t "1"
}

if (Want "atad") {
    Build atad t "modules\iopcore\cdvdman\atad.c" "ata_device_sector_io_internal"
    foreach ($c in "48 65535", "48 65536", "48 65537", "48 131072", "28 256", "28 257") { Test "atad LBA$($c.Replace(' ', ' sectors '))" atad-t $c.Split(" ") }
}

if (Want "smb") {
    Copy-Item (Join-Path $Opl "modules\iopcore\cdvdman\smb.h") $build
    Build smb t "modules\iopcore\cdvdman\smb.c" "USE_CUSTOM_RECV", "ZERO_PKT_ALIGNED", "CLIENT_MAX_XMIT_SIZE", "CLIENT_MAX_RECV_SIZE", "WAITIOSEMA", "SIGNALIOSEMA", "smb_ReadAndX", "smb_ReadFile", "smb_WriteAndX", "smb_WriteFile"
    foreach ($t in "ok", "error", "closed") { Test "smb read $t" smb-t "read", $t }
    foreach ($t in "ok", "error") { Test "smb write $t" smb-t "write", $t }
    # write, read and open results of the SMB client: success, -EIO, 0, failed open
    Build smbvmc t "modules\mcemu\device-smb.c" "DeviceWritePage", "DeviceReadPage"
    foreach ($t in "512 512 1", "-5 -5 1", "0 0 1", "512 512 -1") { Test "smbvmc $t" smbvmc-t ($t -split " ") }
}

if (Want "bgm") {
    Build bgm t "src\sound.c" "bgmLoad"
    Test "bgmLoad default_bgm_path" bgm-t "mass:/MUSIC/100%s.ogg"
    Test "bgmLoad file not Ogg" bgm-t "mass:/bgm.ogg", "notogg"
}

if (Want "guiops") {
    Build guiops t "src\gui.c" "guiDeferUpdate", "guiHandleDeferredOps"
    Test "gui deferred ops, IO waiting" guiops-t
}

if (Want "staleid") {
    Build staleid t "src\menusys.c" "_menuLoadConfig" -Source2 "src\bdmsupport.c" -Funcs2 "bdmGetConfig" -Source3 "src\supportbase.c" -Funcs3 "sbPopulateConfig"
    foreach ($c in "same", "shorter", "removed") { Test "game settings, list $c" staleid-t $c }
}

if (Want "langswitch") {
    Build langswitch t "src\lang.c" "lngFreeFromFile", "lngLoadFont", "lngLoadFromFile", "lngSetGuiValue"
    Test "language file <-> English x3" langswitch-t
}
if (Want "dialock") {
    Build dialock t "src\dia.c" "UI_SPACING_H", "DIA_SCROLL_SPEED", "KEYB_MODE", "KEYB_WIDTH", "KEYB_HEIGHT", "KEYB_ITEMS", "diaDrawBoundingBox", "diaShowKeyb", "diaShowColSel", "diaExecuteDialog"
    foreach ($c in "dialog", "keyboard", "colour") { Test "$c frames, IO swaps theme" dialock-t $c }
}

if (Want "apps") {
    Build memleak apps "src\appsupport.c" "?appFreeLegacyConfig", "appNeedsUpdate", "addAppsLegacyList", "appUpdateItemList", "appFreeList", "appLaunchItem" -Defines "/DAPPS"
    Test "apps list refresh x3 (20 apps)" memleak-apps "refresh", "3", "20"
    Test "app launch with an argument" memleak-apps "launch"
}

if (Want "sboom") {
    Build memleak sb "src\supportbase.c" "game_list_t", "sbReadList" -Defines "/DSB"
    Test "sbReadList, list allocation fails" memleak-sb "300"
}

if (Want "texcache") {
    Build texcache t "src\texcache.c" "load_image_request_t", "cacheLoadImage", "cacheClearItem", "cacheInitCache", "cacheDestroyCache", "cacheGetTexture"
    foreach ($c in "ok", "destroyed", "blocked") { Test "art cache $c" texcache-t $c }
}

if (Want "readfile") {
    Build readfile t "src\util.c" "readFile"
    foreach ($c in "ok", "nomem", "short") { Test "readFile $c" readfile-t $c }
}

if (Want "about") {
    # the YAOPL nightly with all build options, its nosio2man variant without the extra features, an upstream build
    foreach ($v in @(@("nightly", "/DVER=1 /D__RTL /DIGS /DPADEMU"), @("nosio2man", "/DVER=2 /DPADEMU"), @("upstream", "/DVER=3 /D__RTL /DIGS /DPADEMU"))) {
        Build about $v[0] "src\gui.c" "guiShowAbout" -Source2 "include\pggsm.h" -Funcs2 "GSM_VERSION" -Source3 "include\opl.h" -Funcs3 "IGS_VERSION" -Defines $v[1]
        Test "About page $($v[0])" "about-$($v[0])" @()
    }
}

if (Want "udpbdro") {
    $probeDefine = if ((Get-Content -Raw (Join-Path $Opl "src\bdmsupport.c")).Contains("bdmCheckWritable(")) { "/DHAVE_PROBE" } else { "" }
    Build udpbdro t "src\bdmsupport.c" "?bdmCheckWritable", "bdmGetConfig" -Source2 "src\supportbase.c" -Funcs2 "sbCreatePath_name", "sbCreatePath", "sbPopulateConfig" -Source3 "src\util.c" -Funcs3 "checkFile", "openFile" -Defines $probeDefine
    $dir = Join-Path $build "udpbdro"
    foreach ($case in "probe rw", "probe drop", "probe stale", "probe fail", "cfg rw", "cfg drop", "cfg drop-saved", "cfg drop-usbhome", "cfg usb") {
        foreach ($v in "plain", "asan") {
            if (Test-Path $dir) { Remove-Item -Recurse -Force $dir }
            New-Item -ItemType Directory -Force $dir | Out-Null
            $exe = if ($v -eq "plain") { "udpbdro-t" } else { "udpbdro-t-asan" }
            Emit ("{0,-34} {1}: {2}" -f $(if ($v -eq "plain") { "UDPBD $case" } else { "" }), $v.PadRight(5), (Run $exe (@($dir) + $case.Split(" "))))
        }
    }
}

if (Want "openfile") {
    # The folder openFile() makes for a file it creates: <path>, then the folders that are there already
    Build openfile t "src\util.c" "checkFile", "openFile"
    $dir = Join-Path $build "openfile"
    $cases = @(
        @("memory card", "mc0:OPL/CFG/SLUS_123.45.cfg", "mc0:", "mc0:OPL"),
        @("USB, folder missing", "mass0:CFG/SLUS_123.45.cfg", "mass0:"),
        @("USB, folder there", "mass0:CFG/SLUS_123.45.cfg", "mass0:", "mass0:CFG"),
        @("USB, root", "mass0:/conf_opl.cfg", "mass0:"),
        @("HDD", "pfs0:OPL/CFG/SLUS_123.45.cfg", "pfs0:", "pfs0:OPL"),
        @("SMB", "smb0:GAMES\CFG\SLUS_123.45.cfg", "smb0:", "smb0:GAMES"))
    foreach ($case in $cases) {
        foreach ($v in "plain", "asan") {
            if (Test-Path $dir) { Remove-Item -Recurse -Force $dir }
            New-Item -ItemType Directory -Force $dir | Out-Null
            $exe = if ($v -eq "plain") { "openfile-t" } else { "openfile-t-asan" }
            Emit ("{0,-34} {1}: {2}" -f $(if ($v -eq "plain") { "openFile $($case[0])" } else { "" }), $v.PadRight(5), (Run $exe (@($dir) + $case[1..($case.Count - 1)])))
        }
    }
}

if (Want "gamesize") {
    # The paths the game info page reads the size from, on every kind of prefix
    $sizeDefine = if ((Get-Content -Raw (Join-Path $Opl "src\supportbase.c")).Contains("const char *cfgDir)")) { "/DHAVE_CFGDIR" } else { "" }
    Build gamesize t "src\supportbase.c" "sbCreatePath_name", "sbCreatePath", "sbPopulateConfig" -Defines $sizeDefine
    foreach ($c in "iso mass0: /", "iso mass0:GAMES/ /", "iso mmce0:/ /", "iso mmce0:/GAMES/ /", "iso smb0: \", "iso smb0:GAMES\ \", "old mass0: /", "ul mass0: /") {
        Test "Game size $c" gamesize-t $c.Split(" ")
    }
}

if ($Images) {
    Build gameid t "src\supportbase.c" "GetStartupExecName" -Source2 "modules\isofs\isofs.c" -Funcs2 "dirTocEntry", "cdvdman_trimspaces", "cdvdman_locatefile" -Source3 "src\ps2cnf.c" -Funcs3 "CNF_LEN_MAX", "CNFGetToken", "CNFAdvanceLine", "CNFGetKey", "ps2cnfGetBootFile" -Patch 'fopen(path, "r")=>fopen(path, "rb")'
    foreach ($f in Get-ChildItem $Images -File | Where-Object { $_.Extension -in ".iso" }) { Test "gameid $($f.Name)" gameid-t $f.FullName }
}

if ($Expected) {
    $want = @(Get-Content $Expected | Where-Object { $_ -notmatch "^OPL tree:" })
    $got = @($script:lines)
    $bad = 0
    for ($i = 0; $i -lt [Math]::Max($want.Count, $got.Count); $i++) {
        if ($want[$i] -cne $got[$i]) {
            "line $($i + 1) expected: $($want[$i])"
            "line $($i + 1) got:      $($got[$i])"
            $bad++
        }
    }
    if ($bad) { "$bad lines differ from $Expected"; exit 1 }
    "all $($got.Count) lines match $Expected"
}
