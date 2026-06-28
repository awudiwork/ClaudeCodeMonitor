// OptionsDlg.cpp: 实现文件
//

#include "pch.h"
#include "ClaudeCodeMonitor.h"
#include "OptionsDlg.h"
#include "afxdialogex.h"


// COptionsDlg 对话框

IMPLEMENT_DYNAMIC(COptionsDlg, CDialog)

COptionsDlg::COptionsDlg(CWnd* pParent /*=nullptr*/)
    : CDialog(IDD_OPTIONS_DIALOG, pParent)
{
}

COptionsDlg::~COptionsDlg()
{
}

void COptionsDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(COptionsDlg, CDialog)
END_MESSAGE_MAP()


// COptionsDlg 消息处理程序

BOOL COptionsDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // 用当前配置初始化控件
    SetDlgItemText(IDC_STATUS_DIR_EDIT, m_data.status_dir.c_str());
    SetDlgItemInt(IDC_STALE_EDIT, m_data.stale_seconds, FALSE);
    CheckDlgButton(IDC_BLINK_CHECK, m_data.blink ? BST_CHECKED : BST_UNCHECKED);

    return TRUE;  // return TRUE unless you set the focus to a control
}

void COptionsDlg::OnOK()
{
    // 从控件读取配置
    CString dir;
    GetDlgItemText(IDC_STATUS_DIR_EDIT, dir);
    dir.Trim();
    m_data.status_dir = dir.GetString();

    BOOL translated = FALSE;
    int stale = static_cast<int>(GetDlgItemInt(IDC_STALE_EDIT, &translated, FALSE));
    if (translated && stale > 0)
        m_data.stale_seconds = stale;

    m_data.blink = (IsDlgButtonChecked(IDC_BLINK_CHECK) != 0);

    CDialog::OnOK();
}
