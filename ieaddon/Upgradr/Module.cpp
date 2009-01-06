#include "stdafx.h"

// uncomment this to use Visual Leak Detector
// => slightly slower execution, but you gain superb leak detection
//#include "vld.h" 

#include "Module.h"
#include "Scriptsite.h"
#include "ScriptManager.h"
#include "ScriptInstanceManager.h"
#include "ScriptDebugger.h"
#include "HandleManager.h"
#include "FrameManager.h"
#include "BrowserManager.h"
#include "WindowManager.h"
#include "Logger.h"
#include "Services.h"
#include "Database.h"
#include "DialogManager.h"
#include "Server.h"
#include "FileSystem.h"

//#include "Debug.h"

// here are shared resources !

// kazdy resource muze byt vlastnena nejvyse jednim vlaknem => m_Ownership
// kazde vlakno muze mit libovolny pocet resources pokud bezi
// vlakna mohou zadat resources jen se vzrustajicimi ID
// pokud pozadavek nemuze byt uspokojen, thread se uspi na vstupu do CS
class CUpgradrRootImpl : public CUpgradrRoot {
	typedef hash_multimap<DWORD, ESharedResourceId> TResourcesMap;
	typedef hash_map<ESharedResourceId, pair<DWORD, int> > TOwnership;   // pair<threadId, counter>
	typedef hash_map<DWORD, DWORD> TStarveMap;

public:
	CUpgradrRootImpl();
	virtual ~CUpgradrRootImpl();

	virtual bool											Init();
	virtual bool											Done();
	virtual void*											Acquire(ESharedResourceId rid);
	virtual void											Release(ESharedResourceId rid);

	virtual int												ReleaseAll(ESharedResourceId rid);
	virtual void*											AcquireMany(ESharedResourceId rid, int count);

	virtual bool											CheckThreadOwnership(ESharedResourceId rid);
	virtual void*											GetRawDB();

private:
	bool														m_Inited;
	TResourcesMap											m_Resources;
	TOwnership												m_Ownership;
	TStarveMap												m_StarveMap;
	CCS														m_MainCS;
	CCS														m_CS[SR_LAST];

	// shared objects
	CDatabase												m_Database;
	CScriptManager											m_ScriptManager;
	CScriptInstanceManager								m_ScriptInstanceManager;
	CHandleManager											m_HandleManager;
	CLogger													m_Logger;
	CServices												m_Services;
	CScriptDebugger										m_ScriptDebugger;
	CFrameManager											m_FrameManager;
	CWindowManager											m_WindowManager;
	CBrowserManager									   m_BrowserManager;
	CDialogManager											m_DialogManager;
	CServer													m_Server;
	CFileSystem												m_FileSystem;

	static TCHAR											m_WorkspacePath[_MAX_PATH];
};

static CUpgradrRootImpl* gpUpgradrRoot = NULL;
static int giRootInitRefcount = 0;
CCS gRootCS;

void InitRoot()
{
	gRootCS.Enter();
	if (!giRootInitRefcount)
	{
		ATLASSERT(!gpUpgradrRoot);
		gpUpgradrRoot = new CUpgradrRootImpl();
		gpUpgradrRoot->Init();
	}
	giRootInitRefcount++;
	gRootCS.Leave();
}

void DoneRoot()
{
	gRootCS.Enter();
	giRootInitRefcount--;
	if (!giRootInitRefcount)
	{
		ATLASSERT(gpUpgradrRoot);
		gpUpgradrRoot->Done();
		delete gpUpgradrRoot;
	}
	gRootCS.Leave();
}

CUpgradrRoot& GetRoot() 
{ 
	ATLASSERT(gpUpgradrRoot);
	return *gpUpgradrRoot; 
}

static TCHAR* g_acResourceNames[SR_LAST] = {
	_T("Services"),
	_T("ScriptManager"),
	_T("HandleManager"),
	_T("ScriptInstanceManager"),
	_T("FrameManager"),
	_T("WindowManager"),
	_T("BrowserManager"),
	_T("DialogManager"),
	_T("ScriptDebugger"),
	_T("Database"),
	_T("Server"),
	_T("FileSystem"),
	_T("Logger"),
};

CUpgradrRootImpl::CUpgradrRootImpl() : 
m_Inited(false)
{

}

CUpgradrRootImpl::~CUpgradrRootImpl()
{
	Done();
}

bool                                           
CUpgradrRootImpl::Init()
{
	if (m_Inited) return true;

	// debug
#ifdef _DEBUG
	//GetDebugInterfacesModule().m_nIndexBreakAt = 39;
	//g_ComObjectBreak = 10;
#endif

#if (_WIN32_IE >= 0x0300)
	INITCOMMONCONTROLSEX iccx;
	iccx.dwSize = sizeof(iccx);
	iccx.dwICC = ICC_COOL_CLASSES | ICC_BAR_CLASSES | ICC_USEREX_CLASSES; 
	BOOL bRet = ::InitCommonControlsEx(&iccx);
	bRet;
	ATLASSERT(bRet);
#else
	::InitCommonControls();
#endif

	m_Inited = true;
	// CoInternetSetFeatureEnabled(FEATURE_LOCALMACHINE_LOCKDOWN, SET_FEATURE_ON_PROCESS, TRUE); 

	// this is exception, we need running UI thread
	{
		WindowManagerLock windowManager;
		windowManager->Init();
	}

	{
		FileSystemLock fileSystem;
		fileSystem->Init();
	}

	{
		ServerLock server;
		server->Init();
		server->Start();
	}

	// other resources will be inited when they are first asked
	return true;
}

bool                                           
CUpgradrRootImpl::Done()
{
	if (!m_Inited) return true;
	ScriptDebuggerLock()->Done();
	ScriptInstanceManagerLock()->Done();
	ScriptManagerLock()->Done();
	WindowManagerLock()->Done();
	BrowserManagerLock()->Done();
	DialogManagerLock()->Done();
	DatabaseLock()->Done();
	ServerLock()->Done();
	FileSystemLock()->Done();
	m_Inited = false;
	return true;
}

static VOID CALLBACK APCProc(ULONG_PTR dwParam)
{
	DT(TRACE_I(FS(_T("signalled by %08x"), dwParam)));
}

// pri beznem provozu se nesmi TRACOVAT v Acquire a Release
// to vede k zmateni Loggeru, protoze Acquire a Release nejsou reentrantni
void*                                          
CUpgradrRootImpl::Acquire(ESharedResourceId rid)
{
	static bool inAcquire = false;
	// try to enter CS
	m_MainCS.Enter();
	if (inAcquire) DebugBreak(); // this function is NOT reentrant
	inAcquire = true;
	DWORD myThread = GetCurrentThreadId();
	pair<TResourcesMap::iterator, TResourcesMap::iterator> range = m_Resources.equal_range(myThread);
	TResourcesMap::iterator test = range.first;
	ESharedResourceId max = (ESharedResourceId)-1;
	if (test!=range.second)
	{
		if (max>test->second) max = test->second;
		++test;
	}
	if (rid<max)
	{
		TRACE_E(FS(_T("Thread %0X tried to alloc resource %s(%d) after %s(%d)"), myThread, g_acResourceNames[rid], (int)rid, g_acResourceNames[max], (int)max));
		ATLASSERT(0);
	}

	void* res = NULL;
	switch (rid) {
		case SR_DATABASE: res = &m_Database; break;
		case SR_SCRIPTMANAGER: res = &m_ScriptManager; break;
		case SR_HANDLEMANAGER: res = &m_HandleManager; break;
		case SR_LOGGER: res = &m_Logger; break;
		case SR_SERVICES: res = &m_Services; break;
		case SR_SCRIPTDEBUGGER: res = &m_ScriptDebugger; break;
		case SR_SCRIPTINSTANCEMANAGER: res = &m_ScriptInstanceManager; break;
		case SR_FRAMEMANAGER: res = &m_FrameManager; break;
		case SR_WINDOWMANAGER: res = &m_WindowManager; break;
		case SR_BROWSERMANAGER: res = &m_BrowserManager; break;
		case SR_DIALOGMANAGER: res = &m_DialogManager; break;
		case SR_SERVER: res = &m_Server; break;
		case SR_FILESYSTEM: res = &m_FileSystem; break;
	}
	ATLASSERT(res);

	// test if someone owns that resource already
	TOwnership::iterator owner = m_Ownership.find(rid);
	if (owner==m_Ownership.end())
	{
		// well, nobody owns that resource, it is safe to enter
		DT(OutputDebugString(FS(_T("Module[%08X]: Acquire %s"), myThread, g_acResourceNames[rid])));
		
		// register as an owner
		m_Ownership.insert(make_pair(rid, make_pair(myThread, 1)));
		m_Resources.insert(make_pair(myThread, rid));

		owner = m_Ownership.find(rid);

		// enter critical section
		ATLASSERT(m_CS[rid].CanEnter());
		m_CS[rid].Enter(); // it must proceed !!!
	}
	else
	{
		// someone owns that resource
		if (owner->second.first==myThread)
		{
			// already have it
			DT(OutputDebugString(FS(_T("Module[%08X]: Multiple enter into %s (%dx)"), myThread, g_acResourceNames[rid], owner->second.second+1)));

			// increase enter counter
			owner->second.second++;

			// and leave
			inAcquire = false;
			m_MainCS.Leave();
			return res;
		}

		// i'm going to fall asleep
		DT(OutputDebugString(FS(_T("Module[%08X]: Hibernate %s"), myThread, g_acResourceNames[rid])));
		m_StarveMap.insert(make_pair(myThread, 0));
		while (true) // wait until out resource is free
		{
			// leave main CS
			inAcquire = false;
			m_MainCS.Leave();

			// wait for signal, zzZZ
			SleepEx(100, TRUE); // thread is alertable

			m_MainCS.Enter();
			if (inAcquire) DebugBreak(); // this function is NOT reentrant
			inAcquire = true;

			DT(OutputDebugString(FS(_T("Module[%08X]: Reborn %s"), myThread, g_acResourceNames[rid])));

			// is requested resource free ?
			owner = m_Ownership.find(rid);
			if (owner==m_Ownership.end())
			{
				break;
			}
			m_StarveMap[myThread]+=100;
		}
		m_StarveMap.erase(myThread);
		DT(OutputDebugString(FS(_T("Module[%08X]: Thread is alive %s"), myThread, g_acResourceNames[rid])));

		// register as an owner
		m_Ownership.insert(make_pair(rid, make_pair(myThread, 1)));
		m_Resources.insert(make_pair(myThread, rid));

		// enter critical section
		ATLASSERT(m_CS[rid].CanEnter());
		m_CS[rid].Enter(); // it must proceed !!!
	}
	
	inAcquire = false;
	m_MainCS.Leave();
	return res;
}

void                                           
CUpgradrRootImpl::Release(ESharedResourceId rid)
{
	static bool inRelease = false;
	m_MainCS.Enter();
	if (inRelease) DebugBreak(); // this function is NOT reentrant
	inRelease = true;
	DWORD myThread = GetCurrentThreadId();
	TOwnership::iterator ownership = m_Ownership.find(rid);
	ATLASSERT(ownership!=m_Ownership.end());
	--ownership->second.second;
	if (ownership->second.second>0)
	{
		DT(OutputDebugString(FS(_T("Module[%08X]: Release %s (remains %d)"), myThread, g_acResourceNames[rid], ownership->second.second)));
		inRelease = false;
		m_MainCS.Leave();
		return;
	}
	else
	{
		DT(OutputDebugString(FS(_T("Module[%08X]: Release %s"), myThread, g_acResourceNames[rid])));
	}
	m_Ownership.erase(ownership);
	pair<TResourcesMap::iterator, TResourcesMap::iterator> range = m_Resources.equal_range(myThread);
	TResourcesMap::iterator lookup = range.first;
	while (lookup!=range.second)
	{
		if (lookup->second==rid) 
		{
			m_Resources.erase(lookup);
			break;
		}
		++lookup;
	}

	m_CS[rid].Leave(); // nice to leave the section

	// signal most starving thread
	DWORD mostStarving = NULL;
	DWORD amax = 0;
	TStarveMap::iterator si = m_StarveMap.begin();
	while (si!=m_StarveMap.end())
	{
		if (si->second>=amax)
		{
			amax = si->second;
			mostStarving = si->first;
		}
		++si;
	}
	
	if (mostStarving)
	{
		HANDLE hThread = OpenThread(THREAD_SET_CONTEXT, TRUE, mostStarving);
		if (hThread)
		{
			QueueUserAPC(APCProc, hThread, myThread);
			CloseHandle(hThread);
		}
	}
	ATLASSERT(m_Ownership.find(rid)==m_Ownership.end());
	inRelease = false;
	m_MainCS.Leave();
}

int												
CUpgradrRootImpl::ReleaseAll(ESharedResourceId rid)
{
	static bool inReleaseAll = false;
	m_MainCS.Enter();
	if (inReleaseAll) DebugBreak(); // this function is NOT reentrant
	inReleaseAll = true;

	DWORD myThread = GetCurrentThreadId();
	TOwnership::iterator ownership = m_Ownership.find(rid);
	if (ownership==m_Ownership.end()) 
	{
		DT(OutputDebugString(FS(_T("Module[%08X]: ReleaseAll %s (count=0)"), myThread, g_acResourceNames[rid])));
		inReleaseAll = false;
		m_MainCS.Leave();
		return 0;
	}
	DT(OutputDebugString(FS(_T("Module[%08X]: ReleaseAll %s (count=%d)"), myThread, g_acResourceNames[rid], ownership->second.second)));

	int count = ownership->second.second;
	m_Ownership.erase(ownership);
	pair<TResourcesMap::iterator, TResourcesMap::iterator> range = m_Resources.equal_range(myThread);
	TResourcesMap::iterator lookup = range.first;
	while (lookup!=range.second)
	{
		if (lookup->second==rid) 
		{
			m_Resources.erase(lookup);
			break;
		}
		++lookup;
	}
	m_CS[rid].Leave(); // nice to leave the section
	inReleaseAll = false;
	m_MainCS.Leave();
	return count;
}

void*											
CUpgradrRootImpl::AcquireMany(ESharedResourceId rid, int count)
{
	void* res = NULL;
	for (int i=0; i<count; i++)
	{
		res = Acquire(rid);
	}
	return res;
}

bool											
CUpgradrRootImpl::CheckThreadOwnership(ESharedResourceId rid)
{
#ifdef _DEBUG
	DWORD tid = GetCurrentThreadId();
	m_MainCS.Enter();
	TOwnership::iterator i = m_Ownership.find(rid);
	if (i==m_Ownership.end())
	{
		TRACE_E(FS(_T("Resource %s owned by no thread used by thread %08X !!!"), g_acResourceNames[rid], tid));
		DebugBreak();
		m_MainCS.Leave();
		return false;
	}
	if (i->second.first!=tid)
	{
		TRACE_E(FS(_T("Resource %s owned by thread %08X used by thread %08X !!!"), g_acResourceNames[rid], i->second.first, tid));
		DebugBreak();
		m_MainCS.Leave();
		return false;
	}
	m_MainCS.Leave();
#endif
	return true;
}

void*										
CUpgradrRootImpl::GetRawDB()
{
	CheckThreadOwnership(SR_DATABASE);
	return m_Database.GetRaw();
}