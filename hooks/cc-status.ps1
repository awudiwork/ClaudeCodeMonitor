<#
.SYNOPSIS
    TrafficMonitor「Claude Code 状态监控」插件的状态写入助手。

.DESCRIPTION
    由 Claude Code 的 hooks 调用。脚本从 stdin 读取 hook 传入的 JSON，
    取出 session_id 与 cwd，把当前状态写入状态目录下的 <session_id>.txt。
    TrafficMonitor 插件会周期扫描该目录并点亮对应颜色的指示灯。

    状态目录默认 %USERPROFILE%\.claude\cc-status，
    可用环境变量 TM_CC_STATUS_DIR 覆盖（需与插件选项里的"状态文件目录"保持一致）。

.PARAMETER State
    要写入的状态：idle / running / waiting / error / end。
    end 表示会话结束，删除该会话的状态文件。

.EXAMPLE
    '{"session_id":"t1","cwd":"E:\\demo"}' | .\cc-status.ps1 -State running
#>
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('idle', 'running', 'waiting', 'error', 'end')]
    [string]$State
)

$ErrorActionPreference = 'SilentlyContinue'

try {
    # 解析状态目录（环境变量优先，否则用默认路径）
    $dir = $env:TM_CC_STATUS_DIR
    if ([string]::IsNullOrWhiteSpace($dir)) {
        $dir = Join-Path $env:USERPROFILE '.claude\cc-status'
    }
    if (-not (Test-Path -LiteralPath $dir)) {
        New-Item -ItemType Directory -Force -Path $dir | Out-Null
    }

    # 读取 hook 传入的 JSON
    $raw = [Console]::In.ReadToEnd()
    $sid = 'default'
    $cwd = ''
    if (-not [string]::IsNullOrWhiteSpace($raw)) {
        try {
            $obj = $raw | ConvertFrom-Json
            if ($obj.session_id) { $sid = [string]$obj.session_id }
            if ($obj.cwd) { $cwd = [string]$obj.cwd }
        }
        catch { }
    }

    # 清洗会话 id，避免非法文件名字符
    foreach ($c in [System.IO.Path]::GetInvalidFileNameChars()) {
        $sid = $sid.Replace($c, '_')
    }
    if ([string]::IsNullOrWhiteSpace($sid)) { $sid = 'default' }

    $file = Join-Path $dir ($sid + '.txt')

    if ($State -eq 'end') {
        Remove-Item -LiteralPath $file -Force -ErrorAction SilentlyContinue
    }
    else {
        # 两行：第 1 行状态字，第 2 行 cwd（供插件 tooltip 显示）；UTF-8 无 BOM
        $content = $State + "`n" + $cwd
        $enc = New-Object System.Text.UTF8Encoding($false)
        [System.IO.File]::WriteAllText($file, $content, $enc)
    }
}
catch { }

# 永远以 0 退出，避免影响 Claude Code 的正常流程
exit 0
