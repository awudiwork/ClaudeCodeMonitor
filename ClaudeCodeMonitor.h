#pragma once
#include "PluginInterface.h"
#include "StatusLightItem.h"
#include "CompletionLightItem.h"

class CClaudeCodeMonitor : public ITMPlugin
{
private:
    CClaudeCodeMonitor();

public:
    static CClaudeCodeMonitor& Instance();

    // 通过 ITMPlugin 继承
    virtual IPluginItem* GetItem(int index) override;
    virtual void DataRequired() override;
    virtual const wchar_t* GetInfo(PluginInfoIndex index) override;
    virtual OptionReturn ShowOptionsDialog(void* hParent) override;
    virtual void OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data) override;
    virtual const wchar_t* GetTooltipInfo() override;

private:
    CStatusLightItem m_status_item;             // 下灯：当前状态
    CCompletionLightItem m_completion_item;     // 上灯：完成提示

    static CClaudeCodeMonitor m_instance;
};

#ifdef __cplusplus
extern "C" {
#endif
    __declspec(dllexport) ITMPlugin* TMPluginGetInstance();

#ifdef __cplusplus
}
#endif
