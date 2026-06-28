#include "pch.h"
#include "DataManager.h"
#include <fstream>
#include <iterator>
#include <vector>
#include <windows.h>

// 用于获取本模块（DLL）的路径
EXTERN_C IMAGE_DOS_HEADER __ImageBase;

CDataManager CDataManager::m_instance;

CDataManager::CDataManager()
{
}

CDataManager::~CDataManager()
{
    SaveConfig();
}

CDataManager& CDataManager::Instance()
{
    return m_instance;
}

void CDataManager::LoadConfig(const std::wstring& config_dir)
{
    // 计算配置文件路径：<配置目录>\<本DLL文件名>.ini
    HMODULE hModule = reinterpret_cast<HMODULE>(&__ImageBase);
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(hModule, path, MAX_PATH);
    std::wstring module_path = path;
    m_config_path = module_path;
    if (!config_dir.empty())
    {
        size_t index = module_path.find_last_of(L"\\/");
        std::wstring module_file_name = module_path.substr(index + 1);
        m_config_path = config_dir + module_file_name;
    }
    m_config_path += L".ini";

    // 读取状态文件目录
    wchar_t buff[MAX_PATH]{};
    GetPrivateProfileStringW(L"config", L"status_dir", L"", buff, MAX_PATH, m_config_path.c_str());
    m_setting_data.status_dir = buff;
    m_setting_data.stale_seconds = GetPrivateProfileIntW(L"config", L"stale_seconds", 600, m_config_path.c_str());
    if (m_setting_data.stale_seconds <= 0)
        m_setting_data.stale_seconds = 600;
    m_setting_data.blink = GetPrivateProfileIntW(L"config", L"blink", 1, m_config_path.c_str()) != 0;
}

void CDataManager::SaveConfig() const
{
    if (m_config_path.empty())
        return;
    WritePrivateProfileStringW(L"config", L"status_dir", m_setting_data.status_dir.c_str(), m_config_path.c_str());
    wchar_t buff[16];
    swprintf_s(buff, L"%d", m_setting_data.stale_seconds);
    WritePrivateProfileStringW(L"config", L"stale_seconds", buff, m_config_path.c_str());
    WritePrivateProfileStringW(L"config", L"blink", m_setting_data.blink ? L"1" : L"0", m_config_path.c_str());
}

std::wstring CDataManager::ResolveStatusDir() const
{
    if (!m_setting_data.status_dir.empty())
        return m_setting_data.status_dir;
    // 默认 %USERPROFILE%\.claude\cc-status
    wchar_t profile[MAX_PATH]{};
    DWORD len = GetEnvironmentVariableW(L"USERPROFILE", profile, MAX_PATH);
    if (len == 0 || len >= MAX_PATH)
        return std::wstring();
    return std::wstring(profile) + L"\\.claude\\cc-status";
}

// 读取文件全部字节
static std::string ReadFileBytes(const std::wstring& path)
{
    std::ifstream f(path.c_str(), std::ios::binary);
    if (!f)
        return std::string();
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

// UTF-8 转宽字符串
static std::wstring Utf8ToWide(const std::string& s)
{
    if (s.empty())
        return std::wstring();
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring w(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), &w[0], len);
    return w;
}

static std::string Trim(const std::string& s)
{
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos)
        return std::string();
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

// 解析状态文件内容，返回状态并通过 cwd 输出工作目录
static CCState ParseStatusFile(const std::string& raw, std::wstring& cwd)
{
    std::string content = raw;
    // 去除 UTF-8 BOM
    if (content.size() >= 3 &&
        static_cast<unsigned char>(content[0]) == 0xEF &&
        static_cast<unsigned char>(content[1]) == 0xBB &&
        static_cast<unsigned char>(content[2]) == 0xBF)
    {
        content.erase(0, 3);
    }

    // 第 1 行：状态字；第 2 行：cwd
    std::string line0, line1;
    size_t p = content.find_first_of("\r\n");
    line0 = (p == std::string::npos) ? content : content.substr(0, p);
    if (p != std::string::npos)
    {
        size_t q = content.find_first_not_of("\r\n", p);
        if (q != std::string::npos)
        {
            size_t e = content.find_first_of("\r\n", q);
            line1 = (e == std::string::npos) ? content.substr(q) : content.substr(q, e - q);
        }
    }

    cwd = Utf8ToWide(Trim(line1));

    std::string st = Trim(line0);
    if (st == "running") return CCState::Running;
    if (st == "waiting") return CCState::Waiting;
    if (st == "error")   return CCState::Error;
    if (st == "idle")    return CCState::Idle;
    if (st.empty())      return CCState::None;
    return CCState::Idle;    // 文件存在但状态未知，视为有会话
}

static UINT StateStringId(CCState s)
{
    switch (s)
    {
    case CCState::Running: return IDS_STATE_RUNNING;
    case CCState::Waiting: return IDS_STATE_WAITING;
    case CCState::Error:   return IDS_STATE_ERROR;
    case CCState::Idle:    return IDS_STATE_IDLE;
    default:               return IDS_STATE_NONE;
    }
}

void CDataManager::RefreshStatus()
{
    m_phase++;

    std::wstring dir = ResolveStatusDir();
    CCState agg = CCState::None;

    struct SessionInfo { CCState state; std::wstring cwd; };
    std::vector<SessionInfo> sessions;

    if (!dir.empty())
    {
        FILETIME ft_now{};
        GetSystemTimeAsFileTime(&ft_now);
        ULARGE_INTEGER now{};
        now.LowPart = ft_now.dwLowDateTime;
        now.HighPart = ft_now.dwHighDateTime;

        WIN32_FIND_DATAW fd{};
        std::wstring pattern = dir + L"\\*.txt";
        HANDLE hFind = FindFirstFileW(pattern.c_str(), &fd);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    continue;

                // 失效判定：文件最后修改时间距今超过 stale_seconds 则忽略
                ULARGE_INTEGER last{};
                last.LowPart = fd.ftLastWriteTime.dwLowDateTime;
                last.HighPart = fd.ftLastWriteTime.dwHighDateTime;
                if (now.QuadPart > last.QuadPart)
                {
                    unsigned long long age_sec = (now.QuadPart - last.QuadPart) / 10000000ULL;
                    if (age_sec > static_cast<unsigned long long>(m_setting_data.stale_seconds))
                        continue;
                }

                std::wstring file_path = dir + L"\\" + fd.cFileName;
                std::wstring cwd;
                CCState st = ParseStatusFile(ReadFileBytes(file_path), cwd);
                if (st == CCState::None)
                    continue;

                if (static_cast<int>(st) > static_cast<int>(agg))
                    agg = st;
                sessions.push_back({ st, cwd });
            } while (FindNextFileW(hFind, &fd));
            FindClose(hFind);
        }
    }

    m_state = agg;

    // 构建 tooltip 文本
    std::wstring tip = StringRes(StateStringId(agg)).GetString();
    if (!sessions.empty())
    {
        wchar_t cnt[64];
        swprintf_s(cnt, L"  (%zu %s)", sessions.size(), StringRes(IDS_TIP_SESSIONS).GetString());
        tip += cnt;
        for (const auto& s : sessions)
        {
            tip += L"\r\n  - ";
            tip += StringRes(StateStringId(s.state)).GetString();
            if (!s.cwd.empty())
            {
                tip += L"  ";
                tip += s.cwd;
            }
        }
    }
    m_tooltip = tip;
}

const CString& CDataManager::StringRes(UINT id)
{
    auto iter = m_string_table.find(id);
    if (iter != m_string_table.end())
    {
        return iter->second;
    }
    else
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());
        m_string_table[id].LoadString(id);
        return m_string_table[id];
    }
}
