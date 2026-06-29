#pragma once
#include "PluginInterface.h"

// 任务栏/主窗口上的"完成提示"指示灯（上灯），基于当前会话实时状态：
// 蓝(常亮)=有任意会话已答完(空闲/错误)；绿=所有会话都在运行/进行中；熄灭=没有会话
class CCompletionLightItem : public IPluginItem
{
public:
    // 通过 IPluginItem 继承
    virtual const wchar_t* GetItemName() const override;
    virtual const wchar_t* GetItemId() const override;
    virtual const wchar_t* GetItemLableText() const override;
    virtual const wchar_t* GetItemValueText() const override;
    virtual const wchar_t* GetItemValueSampleText() const override;
    virtual bool IsCustomDraw() const override;
    virtual int GetItemWidth() const override;
    virtual void DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) override;
};
