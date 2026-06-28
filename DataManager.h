#pragma once
#include <string>
#include <map>

#define g_data CDataManager::Instance()

// Claude Code 任务状态。枚举值即优先级，多会话汇总时取最大值。
enum class CCState
{
    None = 0,       // 无会话（熄灭）
    Idle = 1,       // 待机/空闲（白/灰）
    Running = 2,    // 运行中（绿）
    Waiting = 3,    // 等待用户授权/输入（黄）
    Error = 4,      // 错误（红）
};

struct SettingData
{
    std::wstring status_dir;        // 状态文件目录，留空表示默认 %USERPROFILE%\.claude\cc-status
    int stale_seconds{ 600 };       // 状态文件多久未更新视为失效（防会话崩溃残留）
    bool blink{ true };             // 等待/错误状态是否闪烁
};

class CDataManager
{
private:
    CDataManager();
    ~CDataManager();

public:
    static CDataManager& Instance();

    void LoadConfig(const std::wstring& config_dir);
    void SaveConfig() const;

    // 扫描状态目录，刷新汇总状态、tooltip 文本与闪烁相位。由 DataRequired 周期调用。
    void RefreshStatus();

    CCState GetState() const { return m_state; }
    int GetPhase() const { return m_phase; }                    // 闪烁相位计数
    const std::wstring& GetTooltip() const { return m_tooltip; }

    const CString& StringRes(UINT id);      // 根据资源 id 获取一个字符串资源（带缓存）

    SettingData m_setting_data;

private:
    // 解析实际使用的状态目录（处理默认值）
    std::wstring ResolveStatusDir() const;

private:
    static CDataManager m_instance;
    std::wstring m_config_path;
    CCState m_state{ CCState::None };
    int m_phase{ 0 };
    std::wstring m_tooltip;
    std::map<UINT, CString> m_string_table;
};
