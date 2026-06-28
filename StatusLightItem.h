#pragma once
#include "PluginInterface.h"

// 任务栏/主窗口上的状态指示灯显示项（自绘一个彩色圆点）
class CStatusLightItem : public IPluginItem
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
