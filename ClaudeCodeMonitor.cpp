#include "pch.h"
#include "ClaudeCodeMonitor.h"
#include "DataManager.h"
#include "OptionsDlg.h"

CClaudeCodeMonitor CClaudeCodeMonitor::m_instance;

CClaudeCodeMonitor::CClaudeCodeMonitor()
{
}

CClaudeCodeMonitor& CClaudeCodeMonitor::Instance()
{
    return m_instance;
}

IPluginItem* CClaudeCodeMonitor::GetItem(int index)
{
    switch (index)
    {
    case 0:
        return &m_status_item;       // 下灯：当前状态
    case 1:
        return &m_completion_item;   // 上灯：完成提示
    default:
        break;
    }
    return nullptr;
}

void CClaudeCodeMonitor::DataRequired()
{
    // 主程序每隔一定时间调用一次，在这里刷新 Claude Code 的状态
    CDataManager::Instance().RefreshStatus();
}

const wchar_t* CClaudeCodeMonitor::GetInfo(PluginInfoIndex index)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    static CString str;
    switch (index)
    {
    case TMI_NAME:
        str.LoadString(IDS_PLUGIN_NAME);
        return str.GetString();
    case TMI_DESCRIPTION:
        str.LoadString(IDS_PLUGIN_DESCRIPTION);
        return str.GetString();
    case TMI_AUTHOR:
        return L"ad";
    case TMI_COPYRIGHT:
        return L"Copyright (C) 2026";
    case TMI_VERSION:
        return L"1.0";
    case TMI_URL:
        return L"https://github.com/zhongyang219/TrafficMonitor";
    default:
        break;
    }
    return L"";
}

ITMPlugin::OptionReturn CClaudeCodeMonitor::ShowOptionsDialog(void* hParent)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    COptionsDlg dlg(CWnd::FromHandle((HWND)hParent));
    dlg.m_data = CDataManager::Instance().m_setting_data;
    if (dlg.DoModal() == IDOK)
    {
        CDataManager::Instance().m_setting_data = dlg.m_data;
        CDataManager::Instance().SaveConfig();
        return ITMPlugin::OR_OPTION_CHANGED;
    }
    return ITMPlugin::OR_OPTION_UNCHANGED;
}

void CClaudeCodeMonitor::OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data)
{
    switch (index)
    {
    case ITMPlugin::EI_CONFIG_DIR:
        // 从配置文件读取配置
        CDataManager::Instance().LoadConfig(std::wstring(data));
        break;
    default:
        break;
    }
}

const wchar_t* CClaudeCodeMonitor::GetTooltipInfo()
{
    return CDataManager::Instance().GetTooltip().c_str();
}

ITMPlugin* TMPluginGetInstance()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    return &CClaudeCodeMonitor::Instance();
}
