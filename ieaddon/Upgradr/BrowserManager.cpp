#include "StdAfx.h"
#include "BrowserManager.h"
#include "UpgradrBHO.h"
#include "UpgradrHelperbar.h"

//#include "Debug.h"

LRESULT													
CBrowserMessageWindow::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return FALSE;
}

CBrowserMessageWindow::CBrowserMessageWindow( IUnknown* browserInterface, CUpgradrBHO* pBHO ) : 
m_RefCount(1), 
m_BrowserInterface(browserInterface), 
m_Helperbar(NULL),
m_ThreadId(0)
{
	CreateMessageWindow();
	SetBHO(pBHO);
}

CBrowserMessageWindow::CBrowserMessageWindow( IUnknown* browserInterface, CUpgradrHelperbar* pHelperbar ) : 
m_RefCount(1), 
m_BrowserInterface(browserInterface), 
m_Helperbar(pHelperbar), 
m_BHO(NULL)
{
	CreateMessageWindow();
}

CBrowserMessageWindow::~CBrowserMessageWindow()
{
	DestroyMessageWindow();
}

bool 
CBrowserMessageWindow::CreateMessageWindow()
{
	ATLASSERT(!m_hWnd);
	CRect rcDefault(0,0,10,10);
	Create(NULL, rcDefault, BROWSER_MESSAGE_WINDOW_NAME);
	ATLASSERT(m_hWnd);
	m_ThreadId = GetCurrentThreadId();
	return true;
}

bool 
CBrowserMessageWindow::DestroyMessageWindow()
{
	ATLASSERT(m_hWnd);
	::DestroyWindow(UnsubclassWindow(TRUE));
	ATLASSERT(!m_hWnd);
	m_ThreadId = 0;
	return true;
}

void 
CBrowserMessageWindow::SetBHO( CUpgradrBHO* pBHO )
{
	m_BHO = pBHO;
	if (!m_BHO) return;

	// re-parent message window
	HWND hwnd;
	CComPtr<IWebBrowser2> browser = m_BHO->GetTopBrowser();
	ATLASSERT(!!browser);
	CHECK(browser->get_HWND((LONG_PTR*)&hwnd));
	SetParent(hwnd);
}
//////////////////////////////////////////////////////////////////////////

CBrowserManager::CBrowserManager():
m_NextId(0)
{
	DT(CREATE_DEBUG_TRACE("BrowserManager"));
}

CBrowserManager::~CBrowserManager()
{
	DT(DELETE_DEBUG_TRACE());
}

TBrowserId
CBrowserManager::AllocBrowserId(IUnknown* browserInterface, CUpgradrBHO* pBHO)
{
	return AllocBrowserId(browserInterface, pBHO, NULL);
}

TBrowserId
CBrowserManager::AllocBrowserId(IUnknown* browserInterface, CUpgradrHelperbar* pHelperbar)
{
	return AllocBrowserId(browserInterface, NULL, pHelperbar);
}

TBrowserId
CBrowserManager::AllocBrowserId(IUnknown* browserInterface, CUpgradrBHO* pBHO, CUpgradrHelperbar* pHelperbar)
{
	DT(TRACE_LI(FS(_T("AllocBrowserId(browserInterface=%08X, BHO=%08X, Helperbar=%08X)"), browserInterface, pBHO, pHelperbar)));
	CHECK_THREAD_OWNERSHIP;
	TBrowserMessageWindowMap::iterator l = FindBrowserId(browserInterface);
	if (l!=m_Browsers.end()) 
	{
		// increase refcount
		l->second->AddRef();

		// fill missing data
		if (pBHO) l->second->SetBHO(pBHO);
		if (pHelperbar) l->second->SetHelperbar(pHelperbar);

		DT(TRACE_LI(FS(_T("... addref to %d and return %d"), l->second->RefCount(), l->first)));
		return l->first;
	}

	// allocate a new browser browserId
	CHECK_THREAD_OWNERSHIP;
	++m_NextId;
	m_Browsers.insert(make_pair(m_NextId, new CBrowserMessageWindow(browserInterface, pBHO)));
	DT(TRACE_LI(FS(_T("... created %d"), m_NextId)));
	return m_NextId;	
}

bool 
CBrowserManager::ReleaseBrowserId(TBrowserId browserId)
{
	DT(TRACE_LI(FS(_T("ReleaseBrowserId(browserId=%d)"), browserId)));
	CHECK_THREAD_OWNERSHIP;
	TBrowserMessageWindowMap::iterator l = m_Browsers.find(browserId);
	ATLASSERT(l!=m_Browsers.end());
	ATLASSERT(l->second->RefCount()>0);
	DT(TRACE_LI(FS(_T("... refcount %d"), l->second->RefCount())));
	if (l->second->DecRef()==0) 
	{
		delete l->second;
		m_Browsers.erase(l);
		DT(TRACE_LI(FS(_T("....... deleted"))));
	}
	return true;
}

TBrowserMessageWindowMap::iterator                                   
CBrowserManager::FindBrowserId(IUnknown* browserInterface)
{
	DT(TRACE_LI(FS(_T("FindBrowserId(browserId=%08X)"), browserInterface)));
	CHECK_THREAD_OWNERSHIP;
	TBrowserMessageWindowMap::iterator i = m_Browsers.begin();
	while (i!=m_Browsers.end())
	{
		if (i->second->GetBrowserInterface()==browserInterface) break;
		++i;
	}
	return i;
}

CBrowserMessageWindow*										
CBrowserManager::FindBrowserMessageWindow(TBrowserId browserId)
{
	CHECK_THREAD_OWNERSHIP;
	TBrowserMessageWindowMap::iterator i = m_Browsers.find(browserId);
	if (i==m_Browsers.end()) return NULL;
	return i->second;
}

bool                                         
CBrowserManager::RequestDOM(TBrowserId browserId)
{
	CHECK_THREAD_OWNERSHIP;
	CBrowserMessageWindow* pBrowserMessageWindow = FindBrowserMessageWindow(browserId);
	if (!pBrowserMessageWindow) return false;
	ATLASSERT(pBrowserMessageWindow->GetBHO());
	pBrowserMessageWindow->GetBHO()->RefreshDOMExplorer();
	return true;
}

bool 
CBrowserManager::IsBrowserThread(DWORD threadId, TBrowserId browserId)
{
	CHECK_THREAD_OWNERSHIP;
	CBrowserMessageWindow* pBrowserMessageWindow = FindBrowserMessageWindow(browserId);
	ATLASSERT(pBrowserMessageWindow);
	if (!pBrowserMessageWindow) return false;
	return pBrowserMessageWindow->GetThreadId()==threadId;
}
