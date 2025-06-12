// Tutorial.h : main header file for the TUTORIAL application
//

#if !defined(AFX_TUTORIAL_H__7C9D0DB7_40F2_495E_AB9C_C2B87506DF2A__INCLUDED_)
#define AFX_TUTORIAL_H__7C9D0DB7_40F2_495E_AB9C_C2B87506DF2A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CTutorialApp:
// See Tutorial.cpp for the implementation of this class
//

class CTutorialApp : public CWinApp
{
public:
	CTutorialApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTutorialApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CTutorialApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TUTORIAL_H__7C9D0DB7_40F2_495E_AB9C_C2B87506DF2A__INCLUDED_)
