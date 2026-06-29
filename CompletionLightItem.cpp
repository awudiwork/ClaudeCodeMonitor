#include "pch.h"
#include "CompletionLightItem.h"
#include "DataManager.h"

const wchar_t* CCompletionLightItem::GetItemName() const
{
    return CDataManager::Instance().StringRes(IDS_COMPLETION_ITEM);
}

const wchar_t* CCompletionLightItem::GetItemId() const
{
    return L"ccDone42x";
}

const wchar_t* CCompletionLightItem::GetItemLableText() const
{
    return L"";
}

const wchar_t* CCompletionLightItem::GetItemValueText() const
{
    return L"";
}

const wchar_t* CCompletionLightItem::GetItemValueSampleText() const
{
    return L"";
}

bool CCompletionLightItem::IsCustomDraw() const
{
    return true;
}

int CCompletionLightItem::GetItemWidth() const
{
    return 20;
}

static COLORREF DimC(COLORREF c, double f)
{
    return RGB(
        static_cast<int>(GetRValue(c) * f),
        static_cast<int>(GetGValue(c) * f),
        static_cast<int>(GetBValue(c) * f));
}

// 画一个实心圆点（带略深描边）
static void DrawFilledDot(HDC hdc, const CRect& dot, COLORREF color)
{
    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_SOLID, 1, DimC(color, 0.6));
    HGDIOBJ old_brush = ::SelectObject(hdc, brush);
    HGDIOBJ old_pen = ::SelectObject(hdc, pen);
    ::Ellipse(hdc, dot.left, dot.top, dot.right, dot.bottom);
    ::SelectObject(hdc, old_brush);
    ::SelectObject(hdc, old_pen);
    ::DeleteObject(brush);
    ::DeleteObject(pen);
}

// 画一个空心圈（熄灭外观）
static void DrawHollowDot(HDC hdc, const CRect& dot, COLORREF ring)
{
    HPEN pen = CreatePen(PS_SOLID, 1, ring);
    HGDIOBJ old_pen = ::SelectObject(hdc, pen);
    HGDIOBJ old_brush = ::SelectObject(hdc, GetStockObject(NULL_BRUSH));
    ::Ellipse(hdc, dot.left, dot.top, dot.right, dot.bottom);
    ::SelectObject(hdc, old_pen);
    ::SelectObject(hdc, old_brush);
    ::DeleteObject(pen);
}

void CCompletionLightItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode)
{
    CDC* pDC = CDC::FromHandle((HDC)hDC);
    if (pDC == nullptr)
        return;

    int side = (w < h) ? w : h;
    int d = static_cast<int>(side * 0.62);
    if (d < 6)
        d = 6;
    int cx = x + w / 2;
    int cy = y + h / 2;
    CRect dot(cx - d / 2, cy - d / 2, cx - d / 2 + d, cy - d / 2 + d);

    pDC->SetBkMode(TRANSPARENT);
    HDC hdc = pDC->GetSafeHdc();

    // 没有任何会话 → 熄灭（空心圈）
    if (!g_data.AnySession())
    {
        COLORREF ring = dark_mode ? RGB(110, 110, 110) : RGB(150, 150, 150);
        DrawHollowDot(hdc, dot, ring);
        return;
    }

    // 有任意会话已答完(空闲/错误) → 蓝（常亮）
    if (g_data.AnyDone())
    {
        COLORREF blue = dark_mode ? RGB(80, 170, 255) : RGB(20, 110, 210);
        DrawFilledDot(hdc, dot, blue);
        return;
    }

    // 所有会话都在运行/进行中 → 绿
    COLORREF green = dark_mode ? RGB(60, 210, 90) : RGB(40, 170, 70);
    DrawFilledDot(hdc, dot, green);
}
