// AppSink.cpp : implementation file
//

#include "stdafx.h"
#include "Tutorial.h"
#include "AppSink.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// AppSink

IMPLEMENT_DYNCREATE(AppSink, CCmdTarget)

AppSink::AppSink()
{
	EnableAutomation();
}

AppSink::~AppSink()
{
}


void AppSink::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CCmdTarget::OnFinalRelease();
}


void AppSink::OnScenarioNew(BSTR path)
{
	AfxMessageBox("New Scenario Created",MB_OK | MB_ICONINFORMATION);
}


BEGIN_MESSAGE_MAP(AppSink, CCmdTarget)
	//{{AFX_MSG_MAP(AppSink)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(AppSink, CCmdTarget)
	//{{AFX_DISPATCH_MAP(AppSink)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		DISP_FUNCTION_ID(AppSink,"OnScenarioNew",1,OnScenarioNew,VTS_NONE,VTS_BSTR)

	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IAppSink to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {7282B843-78AB-41B7-A3F6-622F28CFA57A}
static const IID IID_IAppSink =
{ 0x7282b843, 0x78ab, 0x41b7, { 0xa3, 0xf6, 0x62, 0x2f, 0x28, 0xcf, 0xa5, 0x7a } };

BEGIN_INTERFACE_MAP(AppSink, CCmdTarget)
	INTERFACE_PART(AppSink, DIID_IAgSTKXApplicationEvents, Dispatch)
END_INTERFACE_MAP()

/////////////////////////////////////////////////////////////////////////////
// AppSink message handlers
