#include "pch.h"
#include "StatusLightItem.h"
#include "DataManager.h"

const wchar_t* CStatusLightItem::GetItemName() const
{
    return CDataManager::Instance().StringRes(IDS_STATUS_ITEM);
}

const wchar_t* CStatusLightItem::GetItemId() const
{
    // 稳定且唯一的 ID，主程序用它保存/恢复显示项配置
    return L"ccStat42x";
}

const wchar_t* CStatusLightItem::GetItemLableText() const
{
    return L"";
}

const wchar_t* CStatusLightItem::GetItemValueText() const
{
    return L"";
}

const wchar_t* CStatusLightItem::GetItemValueSampleText() const
{
    return L"";
}

bool CStatusLightItem::IsCustomDraw() const
{
    return true;
}

int CStatusLightItem::GetItemWidth() const
{
    // 96DPI 下的宽度，主程序会按系统 DPI 自动放大
    return 20;
}

// 根据状态与深浅色返回指示灯颜色
static COLORREF StateColor(CCState s, bool dark)
{
    switch (s)
    {
    case CCState::Running: return dark ? RGB(60, 210, 90)  : RGB(40, 170, 70);
    case CCState::Waiting: return dark ? RGB(245, 200, 40) : RGB(220, 170, 10);
    case CCState::Error:   return dark ? RGB(240, 80, 70)  : RGB(215, 50, 40);
    case CCState::Idle:    return dark ? RGB(180, 180, 180): RGB(120, 120, 120);
    default:               return dark ? RGB(110, 110, 110): RGB(150, 150, 150);   // None
    }
}

// 按比例降低亮度
static COLORREF Dim(COLORREF c, double f)
{
    return RGB(
        static_cast<int>(GetRValue(c) * f),
        static_cast<int>(GetGValue(c) * f),
        static_cast<int>(GetBValue(c) * f));
}

void CStatusLightItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode)
{
    CDC* pDC = CDC::FromHandle((HDC)hDC);
    if (pDC == nullptr)
        return;

    CCState state = g_data.GetState();

    // 圆点直径：取显示区域短边的约 0.62，最小 6 像素
    int side = (w < h) ? w : h;
    int d = static_cast<int>(side * 0.62);
    if (d < 6)
        d = 6;
    int cx = x + w / 2;
    int cy = y + h / 2;
    CRect dot(cx - d / 2, cy - d / 2, cx - d / 2 + d, cy - d / 2 + d);

    pDC->SetBkMode(TRANSPARENT);

    if (state == CCState::None)
    {
        // 无会话：画一个空心暗灰圈表示"熄灭"（用原始 GDI 以便选入空画刷）
        COLORREF ring = StateColor(state, dark_mode);
        HDC hdc = pDC->GetSafeHdc();
        HPEN pen = CreatePen(PS_SOLID, 1, ring);
        HGDIOBJ old_pen = ::SelectObject(hdc, pen);
        HGDIOBJ old_brush = ::SelectObject(hdc, GetStockObject(NULL_BRUSH));
        ::Ellipse(hdc, dot.left, dot.top, dot.right, dot.bottom);
        ::SelectObject(hdc, old_pen);
        ::SelectObject(hdc, old_brush);
        ::DeleteObject(pen);
        return;
    }

    COLORREF color = StateColor(state, dark_mode);

    // 等待/错误状态：在奇数相位降低亮度，形成闪烁效果
    if (g_data.m_setting_data.blink &&
        (state == CCState::Waiting || state == CCState::Error) &&
        (g_data.GetPhase() % 2 == 1))
    {
        color = Dim(color, 0.35);
    }

    CBrush brush(color);
    CPen pen(PS_SOLID, 1, Dim(color, 0.6));     // 略深的描边，增强边缘
    CBrush* ob = pDC->SelectObject(&brush);
    CPen* op = pDC->SelectObject(&pen);
    pDC->Ellipse(dot);
    pDC->SelectObject(ob);
    pDC->SelectObject(op);
}
