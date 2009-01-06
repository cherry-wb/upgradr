#pragma once
#include "ListCtrl.h"
#include "Logger.h"

#define IDC_LISTCTRL                              1000

// TODO: restore column widths
// TODO: copy to clipboard
// TODO: don't track new items (option)
class CConsoleWindow : public CWindowImpl<CConsoleWindow> {
public:
	CConsoleWindow();
	virtual ~CConsoleWindow();

	DECLARE_WND_CLASS(CONSOLE_WINDOW_CLASS_NAME)

	BEGIN_MSG_MAP(CConsoleWindow)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_WINDOWPOSCHANGED, OnWindowPosChanged)
		REFLECT_NOTIFICATIONS()
	END_MSG_MAP()

	virtual LRESULT                             	OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual LRESULT                             	OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual LRESULT                             	OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	virtual void                                	Refresh();
	virtual bool                                 SetLogger(CLogger* logger);

protected:
	virtual void                                 Layout();
	virtual void                                 InitList();

private:
	CLogger*                                     m_Logger;
	CLoggerConsole											m_LoggerConsole; ///< logger object
	CImageList                                   m_ImageList;
	CFont                                        m_Font;
};
