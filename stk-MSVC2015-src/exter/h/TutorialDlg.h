// TutorialDlg.h : header file
//
//{{AFX_INCLUDES()
#include "aguiax2dcntrl.h"
#include "aguiaxvocntrl.h"
#include "agstkxapplication.h"
#include "agexeccmdresult.h"
//}}AFX_INCLUDES
#include "AppSink.h"

#if !defined(AFX_TUTORIALDLG_H__642D303C_9542_4F1B_A6A9_05C5E8676B3F__INCLUDED_)
#define AFX_TUTORIALDLG_H__642D303C_9542_4F1B_A6A9_05C5E8676B3F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CTutorialDlg dialog

class CTutorialDlg : public CDialog
{
// Construction
public:
	CTutorialDlg(CWnd* pParent = NULL);	// standard constructor
    ~CTutorialDlg();

// Dialog Data
	//{{AFX_DATA(CTutorialDlg)
	enum { IDD = IDD_TUTORIAL_DIALOG };
	CString	m_editContents;
	CAgUiAx2DCntrl	m_2DControl;
	CAgUiAxVOCntrl	m_VOControl;
	//}}AFX_DATA
	CAgSTKXApplication	m_STKApplication;
	AppSink				m_AppSink;
	DWORD				m_dwCookie;


	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTutorialDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CTutorialDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnButton1();
	afx_msg void OnButton2();
	afx_msg void OnButton3();
	afx_msg void OnDblClick2dcontrol1();
	afx_msg void OnMouseMoveVocontrol1(short Button, short Shift, long x, long y);
	DECLARE_EVENTSINK_MAP()
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TUTORIALDLG_H__642D303C_9542_4F1B_A6A9_05C5E8676B3F__INCLUDED_)
