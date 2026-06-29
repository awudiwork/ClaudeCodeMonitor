# Claude Code 状态监控插件（ClaudeCodeMonitor）

TrafficMonitor 插件：在任务栏（及主窗口）显示一盏**状态指示灯**，实时反映本机 Claude Code 的任务状态。

| 指示灯 | 状态 | 含义 |
|---|---|---|
| ⚪ 灰/白 | Idle | 待机/空闲，等待你的下一条指令 |
| 🟢 绿 | Running | Claude 正在处理任务 |
| 🟡 黄（闪烁） | Waiting | 正在等待你授权（权限确认）或输入 |
| 🔴 红（闪烁） | Error | 发生错误（API 错误 / 工具失败 / 权限被拒） |
| ⚫ 空心圈 | None | 当前没有活动会话 |

多个会话同时开着时，按优先级 **错误 > 等待 > 运行 > 空闲** 汇总成一盏灯。鼠标悬停指示灯可在 tooltip 看到各会话明细。

## 两个指示灯

插件提供**两个显示项**，可分别加到任务栏（双行布局下会上下叠放）：

**下灯「Claude Code 状态」** —— 当前状态（即上表）：⚪空闲 / 🟢运行 / 🟡等授权(闪) / 🔴错误(闪) / ⚫无会话。

**上灯「Claude Code 完成提示」** —— 基于**当前会话实时状态**（不锁存）：

| 上灯 | 含义 |
|---|---|
| 🔵 蓝（常亮） | **至少有一个会话已答完**(空闲/错误) → 轮到你了 |
| 🟢 绿 | **有会话，且全部在运行/进行中**（没有一个空闲） |
| ⚫ 空心圈 | 没有会话 |

- 判定优先级：**有会话答完 → 蓝；否则全在跑 → 绿；否则灭。**
- 一个会话在跑→🟢；它答完→🔵（停在空闲就一直蓝）；你再发问→🟢。
- 等授权(黄)算"进行中"→上灯仍🟢（"等授权"由下灯的黄表达）。
- 蓝灯保持到该空闲会话**失效为止**（默认约 10 分钟，可在选项调失效秒数）。
- 只想用一个灯也行：只加下灯 = 纯当前状态；只加上灯 = 工作中/已答完 概览。

---

## 工作原理

TrafficMonitor 插件是被加载进 `TrafficMonitor.exe` 的 DLL，而 Claude Code 跑在独立终端进程里，两者通过**状态文件**通信：

```
Claude Code ──(hooks 触发)──▶  %USERPROFILE%\.claude\cc-status\<session_id>.txt
                                                │ 每个监控周期读取
                                                ▼
                       TrafficMonitor 插件 → 点亮对应颜色的圆点
```

- Claude Code 的每个生命周期事件触发 `cc-status.ps1`，把状态写入 `<会话id>.txt`。
- 插件在 `DataRequired()` 周期扫描该目录，按优先级汇总后自绘指示灯。
- 文件最后修改时间超过「失效秒数」（默认 600s）视为失效，防止会话异常退出后残留。

---

## 安装步骤

### 1. 构建插件

本工程是**自包含**的（已内置 `include\PluginInterface.h`），可独立构建，无需 TrafficMonitor 主仓库。

**方式 A：GitHub Actions（推荐，本机没装 MFC 也能用）**
把本文件夹作为一个仓库推送到 GitHub，自带的工作流 `.github/workflows/build.yml` 会自动在
`windows-latest`（自带 VS2022 + MFC）上构建，产出 x64 / Win32 两个 DLL。
在仓库 **Actions** 页面进入对应运行，底部 **Artifacts** 下载 `ClaudeCodeMonitor_x64`（按你的 TrafficMonitor 位数选择）。

**方式 B：本地 Visual Studio 2022**
需安装「适用于最新 v143 生成工具的 C++ MFC (x86 和 x64)」组件。直接用 VS 打开 `ClaudeCodeMonitor.vcxproj`，
选 `Release | x64` 生成。命令行：
```powershell
msbuild ClaudeCodeMonitor.vcxproj -p:Configuration=Release -p:Platform=x64 -p:platformToolset=v143
```
产物：`Bin\x64\Release\ClaudeCodeMonitor.dll`（Win32 为 `Bin\Win32\Release\ClaudeCodeMonitor.dll`）。

### 2. 部署插件
把 `ClaudeCodeMonitor.dll` 复制到 TrafficMonitor 安装目录的 `plugins\` 子目录，重启 TrafficMonitor。
- 在「选项 → 插件」中确认插件已加载。
- 在「任务栏设置」或「主窗口设置」中，把显示项「Claude Code 状态」勾选/拖入显示列表。

### 3. 配置 Claude Code hooks

**① 复制脚本**：把 `hooks\cc-status.ps1` 复制到 `%USERPROFILE%\.claude\`（即 `C:\Users\<你>\.claude\cc-status.ps1`）。

**② 配置 hooks**：编辑 `%USERPROFILE%\.claude\settings.json`，把下面整个 `"hooks": { ... }` 块加进最外层对象里
（若文件已有别的配置，在最后一项后面加个逗号再粘贴；若已有 `hooks`，按事件名合并，不要整体覆盖）：

```json
  "hooks": {
    "SessionStart": [
      { "hooks": [ { "type": "command", "shell": "powershell", "command": "Set-ExecutionPolicy -Scope Process Bypass -Force; & \"$HOME/.claude/cc-status.ps1\" -State idle" } ] }
    ],
    "UserPromptSubmit": [
      { "hooks": [ { "type": "command", "shell": "powershell", "command": "Set-ExecutionPolicy -Scope Process Bypass -Force; & \"$HOME/.claude/cc-status.ps1\" -State running" } ] }
    ],
    "PreToolUse": [
      { "matcher": "*", "hooks": [ { "type": "command", "shell": "powershell", "command": "Set-ExecutionPolicy -Scope Process Bypass -Force; & \"$HOME/.claude/cc-status.ps1\" -State running" } ] }
    ],
    "PostToolUse": [
      { "matcher": "*", "hooks": [ { "type": "command", "shell": "powershell", "command": "Set-ExecutionPolicy -Scope Process Bypass -Force; & \"$HOME/.claude/cc-status.ps1\" -State running" } ] }
    ],
    "Notification": [
      { "matcher": "permission_prompt", "hooks": [ { "type": "command", "shell": "powershell", "command": "Set-ExecutionPolicy -Scope Process Bypass -Force; & \"$HOME/.claude/cc-status.ps1\" -State waiting" } ] },
      { "matcher": "idle_prompt", "hooks": [ { "type": "command", "shell": "powershell", "command": "Set-ExecutionPolicy -Scope Process Bypass -Force; & \"$HOME/.claude/cc-status.ps1\" -State idle" } ] }
    ],
    "PermissionRequest": [
      { "hooks": [ { "type": "command", "shell": "powershell", "command": "Set-ExecutionPolicy -Scope Process Bypass -Force; & \"$HOME/.claude/cc-status.ps1\" -State waiting" } ] }
    ],
    "Stop": [
      { "hooks": [ { "type": "command", "shell": "powershell", "command": "Set-ExecutionPolicy -Scope Process Bypass -Force; & \"$HOME/.claude/cc-status.ps1\" -State idle" } ] }
    ],
    "StopFailure": [
      { "hooks": [ { "type": "command", "shell": "powershell", "command": "Set-ExecutionPolicy -Scope Process Bypass -Force; & \"$HOME/.claude/cc-status.ps1\" -State error" } ] }
    ],
    "SessionEnd": [
      { "hooks": [ { "type": "command", "shell": "powershell", "command": "Set-ExecutionPolicy -Scope Process Bypass -Force; & \"$HOME/.claude/cc-status.ps1\" -State end" } ] }
    ]
  }
```

**③ 生效**：保存后**重启 Claude Code**，或在会话里执行 `/hooks` 确认已加载。

> 命令里的 `Set-ExecutionPolicy -Scope Process Bypass` 前缀用于绕过 PowerShell 执行策略，确保脚本能跑。
> 上面这段与 `hooks/settings.sample.json` 内容一致，直接复制本块即可。

---

## 状态 → Hook 事件 对照

| 状态 | 触发的 hook 事件 |
|---|---|
| idle | `SessionStart`、`Stop`、`Notification`(idle_prompt) |
| running | `UserPromptSubmit`、`PreToolUse`、`PostToolUse`（同意授权后靠它把黄灯拉回绿） |
| waiting | `PermissionRequest`、`Notification`(permission_prompt) |
| error | `StopFailure`（仅整轮因 API 错误失败） |
| （删除文件）| `SessionEnd` |

> ⚠️ 红色**只**用 `StopFailure`（整轮因 API 错误失败，如 rate_limit / overloaded / 认证失败）。
> **不要**把 `PostToolUseFailure` 映射成红色——普通工具非零退出（编译失败、测试没过、grep 无匹配等）都会触发它，会导致正常干活时红灯乱闪。
> 也**不要**用 `async: true`——异步 hook 不保证执行顺序，`running` 可能盖掉 `Stop` 写的 `idle`，导致灯卡在运行中。

---

## 选项设置

在「选项 → 插件 → ClaudeCodeMonitor → 设置」中可配置：

- **状态文件目录**：留空＝默认 `%USERPROFILE%\.claude\cc-status`。若自定义，需同时给 `cc-status.ps1`
  设置环境变量 `TM_CC_STATUS_DIR` 指向同一目录。
- **失效秒数**：状态文件多久未更新视为失效，默认 600。
- **等待/错误时闪烁**：默认开启。

---

## 验证

不依赖 Claude，手动验证桥路是否打通。**注意**：要用 `powershell -File` 开新进程喂 stdin，
不能用 `| & 脚本.ps1`（那样 JSON 会被当成管道参数，报 ParameterBindingException）：
```powershell
'{"session_id":"t1","cwd":"E:\demo"}' | powershell -NoProfile -ExecutionPolicy Bypass -File "$HOME\.claude\cc-status.ps1" -State running
# 任务栏圆点应变绿；依次把 -State 换成 waiting / error / idle 看黄/红/灰
'{"session_id":"t1"}' | powershell -NoProfile -ExecutionPolicy Bypass -File "$HOME\.claude\cc-status.ps1" -State end   # 熄灭
```

查看当前所有会话状态文件：
```powershell
Get-ChildItem "$HOME\.claude\cc-status\" -Filter *.txt | ForEach-Object { "{0}  =>  {1}" -f $_.Name, (Get-Content $_.FullName -TotalCount 1) }
```

随后开一个真实 Claude Code 会话：提交提问→绿，请求授权→黄，回答完毕→灰。

---

## 故障排查

- **灯一直熄灭（空心圈）**：检查 `%USERPROFILE%\.claude\cc-status\` 是否生成了 `*.txt`；没有则说明 hooks 未生效，用 `/hooks` 检查配置。
- **灯一直绿不熄**：确认 `Stop` 事件已配置；或会话异常退出残留，等过了「失效秒数」会自动熄灭。
- **自定义目录后不亮**：插件选项里的目录与脚本的 `TM_CC_STATUS_DIR` 必须一致。
