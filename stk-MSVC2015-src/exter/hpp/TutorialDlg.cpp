// TutorialDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Tutorial.h"
#include "TutorialDlg.h"
#include "agpickinfodata.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTutorialDlg dialog

CTutorialDlg::CTutorialDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTutorialDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTutorialDlg)
	m_editContents = _T("");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CTutorialDlg::~CTutorialDlg()
{
    LPUNKNOWN pUnkSink = m_AppSink.GetIDispatch(FALSE);
    AfxConnectionUnadvise(
	    m_STKApplication, 
	    DIID_IAgSTKXApplicationEvents, 
	    pUnkSink, 
	    FALSE, 
	    m_dwCookie);
}

void CTutorialDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTutorialDlg)
	DDX_Control(pDX, IDC_2DCONTROL1, m_2DControl);
	DDX_Control(pDX, IDC_VOCONTROL1, m_VOControl);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CTutorialDlg, CDialog)
	//{{AFX_MSG_MAP(CTutorialDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, OnButton1)
	ON_BN_CLICKED(IDC_BUTTON2, OnButton2)
	ON_BN_CLICKED(IDC_BUTTON3, OnButton3)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTutorialDlg message handlers

BOOL CTutorialDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	if (!m_STKApplication.CreateDispatch("STKX9.Application"))
	{
	TRACE0("Failed to create STK Application\n");
		return FALSE;
	}

	// m_AppSink is the object that will response to events (such as 
	// "OnScenarioNew") sent
	// by the STK/X Application object
	LPUNKNOWN pUnkSink = m_AppSink.GetIDispatch(FALSE);
	AfxConnectionAdvise(m_STKApplication, DIID_IAgSTKXApplicationEvents, pUnkSink, FALSE, &m_dwCookie);

	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CTutorialDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CTutorialDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CTutorialDlg::OnButton1() 
{
	this->m_VOControl.GetApplication().ExecuteCommand("New / Scenario Test");
}

void CTutorialDlg::OnButton2() 
{
	this->m_2DControl.ZoomIn();
}

void CTutorialDlg::OnButton3() 
{
	this->m_2DControl.ZoomOut();
}

BEGIN_EVENTSINK_MAP(CTutorialDlg, CDialog)
    //{{AFX_EVENTSINK_MAP(CTutorialDlg)
	ON_EVENT(CTutorialDlg, IDC_2DCONTROL1, -601 /* DblClick */, OnDblClick2dcontrol1, VTS_NONE)
	ON_EVENT(CTutorialDlg, IDC_VOCONTROL1, -606 /* MouseMove */, OnMouseMoveVocontrol1, VTS_I2 VTS_I2 VTS_I4 VTS_I4)
	//}}AFX_EVENTSINK_MAP
END_EVENTSINK_MAP()

void CTutorialDlg::OnDblClick2dcontrol1() 
{
	MessageBox("2D map double-clicked", NULL, MB_OK | MB_ICONINFORMATION); 
}

void CTutorialDlg::OnMouseMoveVocontrol1(short Button, short Shift, long x, long y) 
{
	CAgPickInfoData pickData = m_VOControl.PickInfo(x,y);

	if(pickData.GetIsLatLonAltValid())
	{
		char number[255];
		sprintf_s(number,"%f",pickData.GetLat());
		CString lat(number);
		sprintf_s(number,"%f",pickData.GetLon());
		CString lon(number);
		sprintf_s(number,"%f",pickData.GetAlt());
		CString alt(number);
		m_editContents="Lat: "+lat+"\r\nLon: "+lon+"\r\nAlt: "+alt;

		GetDlgItem(IDC_LLA_STATIC)->SetWindowText(m_editContents);
	}
	else
	{
		GetDlgItem(IDC_LLA_STATIC)->SetWindowText("");
	}
	
}

