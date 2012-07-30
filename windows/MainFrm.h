/*
 * Copyright (C) 2001-2006 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#if !defined(MAIN_FRM_H)
#define MAIN_FRM_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "HubFrame.h"
#include "../client/TimerManager.h"
#include "../client/HttpConnection.h"
#include "../client/FavoriteManager.h"
#include "../client/QueueManagerListener.h"
#include "../client/Util.h"
#include "../client/LogManager.h"
#include "../client/version.h"
#include "../client/Client.h"
#include "../client/ShareManager.h"
#include "../client/DownloadManager.h"
#include "../client/SettingsManager.h"
#include "../client/WebServerManager.h"
#include "../client/AdlSearch.h"
#include "PopupManager.h"

#include "FlatTabCtrl.h"
#include "SingleInstance.h"
#include "TransferView.h"
#include "WinUtil.h"

#define QUICK_SEARCH_MAP 20

class MainFrame : public CMDIFrameWindowImpl<MainFrame>, public CUpdateUI<MainFrame>,
		public CMessageFilter, public CIdleHandler, public CSplitterImpl<MainFrame, false>, public Thread,
		private TimerManagerListener, private HttpConnectionListener, private QueueManagerListener,
		private LogManagerListener, private WebServerListener
{
public:
	MainFrame();
	~MainFrame();
	DECLARE_FRAME_WND_CLASS(_T(APPNAME), IDR_MAINFRAME)

	CMDICommandBarCtrl m_CmdBar;

	struct Popup {
		tstring Title;
		tstring Message;
		int Icon;
	};

	enum {
		DOWNLOAD_LISTING,
		BROWSE_LISTING,
		STATS,
		AUTO_CONNECT,
		PARSE_COMMAND_LINE,
		VIEW_FILE_AND_DELETE, 
		SET_STATUSTEXT,
		STATUS_MESSAGE,
		SHOW_POPUP,
		REMOVE_POPUP,
		SET_PM_TRAY_ICON
	};

	BOOL PreTranslateMessage(MSG* pMsg)
	{
		if((pMsg->message >= WM_MOUSEFIRST) && (pMsg->message <= WM_MOUSELAST))
			ctrlLastLines.RelayEvent(pMsg);

		if (!IsWindow())
			return FALSE;

		if(CMDIFrameWindowImpl<MainFrame>::PreTranslateMessage(pMsg))
			return TRUE;
		
		HWND hWnd = MDIGetActive();
		if(hWnd != NULL)
			return (BOOL)::SendMessage(hWnd, WM_FORWARDMSG, 0, (LPARAM)pMsg);

		return FALSE;
	}
	
	BOOL OnIdle()
	{
		UIUpdateToolBar();
		return FALSE;
	}
	typedef CSplitterImpl<MainFrame, false> splitterBase;
	BEGIN_MSG_MAP(MainFrame)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(FTM_SELECTED, onSelected)
		MESSAGE_HANDLER(FTM_ROWS_CHANGED, onRowsChanged)
		MESSAGE_HANDLER(WM_APP+242, onTrayIcon)
		MESSAGE_HANDLER(WM_DESTROY, onDestroy)
		MESSAGE_HANDLER(WM_SIZE, onSize)
		MESSAGE_HANDLER(WM_ENDSESSION, onEndSession)
		MESSAGE_HANDLER(trayMessage, onTray)
		MESSAGE_HANDLER(WM_COPYDATA, onCopyData)
		MESSAGE_HANDLER(WMU_WHERE_ARE_YOU, onWhereAreYou)
		MESSAGE_HANDLER(WM_ACTIVATEAPP, onActivateApp)
		MESSAGE_HANDLER(WM_APPCOMMAND, onAppCommand)
		MESSAGE_HANDLER(IDC_REBUILD_TOOLBAR, OnCreateToolbar)
		MESSAGE_HANDLER(WEBSERVER_SOCKET_MESSAGE, onWebServerSocket)
		COMMAND_ID_HANDLER(ID_APP_EXIT, OnFileExit)
		COMMAND_ID_HANDLER(ID_FILE_SETTINGS, OnFileSettings)
		COMMAND_ID_HANDLER(IDC_MATCH_ALL, onMatchAll)
		COMMAND_ID_HANDLER(ID_VIEW_TOOLBAR, OnViewToolBar)
		COMMAND_ID_HANDLER(ID_VIEW_STATUS_BAR, OnViewStatusBar)
		COMMAND_ID_HANDLER(ID_VIEW_TRANSFER_VIEW, OnViewTransferView)
		COMMAND_ID_HANDLER(ID_GET_TTH, onGetTTH)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		COMMAND_ID_HANDLER(ID_WINDOW_CASCADE, OnWindowCascade)
		COMMAND_ID_HANDLER(ID_WINDOW_TILE_HORZ, OnWindowTile)
		COMMAND_ID_HANDLER(ID_WINDOW_TILE_VERT, OnWindowTileVert)
		COMMAND_ID_HANDLER(ID_WINDOW_ARRANGE, OnWindowArrangeIcons)
		COMMAND_ID_HANDLER(IDC_RECENTS, onOpenWindows)
		COMMAND_ID_HANDLER(ID_FILE_CONNECT, onOpenWindows)
		COMMAND_ID_HANDLER(ID_FILE_SEARCH, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_FAVORITES, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_FAVUSERS, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_NOTEPAD, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_QUEUE, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_SEARCH_SPY, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_FILE_ADL_SEARCH, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_NET_STATS, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_FINISHED, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_FINISHED_UL, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_UPLOAD_QUEUE, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_CDMDEBUG_WINDOW, onOpenWindows)
		COMMAND_ID_HANDLER(IDC_AWAY, onAway)
		COMMAND_ID_HANDLER(IDC_LIMITER, onLimiter)
		COMMAND_ID_HANDLER(IDC_HELP_HOMEPAGE, onLink)
		COMMAND_ID_HANDLER(IDC_HELP_DONATE, onLink)
		COMMAND_ID_HANDLER(IDC_HELP_GEOIPFILE, onLink)
		COMMAND_ID_HANDLER(IDC_HELP_TRANSLATIONS, onLink)
		COMMAND_ID_HANDLER(IDC_HELP_FAQ, onLink)
		COMMAND_ID_HANDLER(IDC_HELP_DISCUSS, onLink)
		COMMAND_ID_HANDLER(IDC_OPEN_FILE_LIST, onOpenFileList)
		COMMAND_ID_HANDLER(IDC_OPEN_MY_LIST, onOpenFileList)
		COMMAND_ID_HANDLER(IDC_TRAY_SHOW, onAppShow)
		COMMAND_ID_HANDLER(ID_WINDOW_MINIMIZE_ALL, onWindowMinimizeAll)
		COMMAND_ID_HANDLER(ID_WINDOW_RESTORE_ALL, onWindowRestoreAll)
		COMMAND_ID_HANDLER(IDC_SHUTDOWN, onShutDown)
		COMMAND_ID_HANDLER(IDC_UPDATE, onUpdate)
		COMMAND_ID_HANDLER(IDC_DISABLE_SOUNDS, onDisableSounds)
		COMMAND_ID_HANDLER(IDC_CLOSE_DISCONNECTED, onCloseWindows)
		COMMAND_ID_HANDLER(IDC_CLOSE_ALL_PM, onCloseWindows)
		COMMAND_ID_HANDLER(IDC_CLOSE_ALL_OFFLINE_PM, onCloseWindows)
		COMMAND_ID_HANDLER(IDC_CLOSE_ALL_DIR_LIST, onCloseWindows)
		COMMAND_ID_HANDLER(IDC_CLOSE_ALL_SEARCH_FRAME, onCloseWindows)
		COMMAND_ID_HANDLER(IDC_OPEN_DOWNLOADS, onOpenDownloads)
		COMMAND_ID_HANDLER(IDC_REFRESH_FILE_LIST, onRefreshFileList)
		COMMAND_ID_HANDLER(ID_FILE_QUICK_CONNECT, onQuickConnect)
		COMMAND_ID_HANDLER(IDC_HASH_PROGRESS, onHashProgress)
		COMMAND_ID_HANDLER(ID_TOGGLE_QSEARCH, OnViewQuickSearchBar)
		NOTIFY_CODE_HANDLER(TTN_GETDISPINFO, onGetToolTip)
		CHAIN_MDI_CHILD_COMMANDS()
		CHAIN_MSG_MAP(CUpdateUI<MainFrame>)
		CHAIN_MSG_MAP(CMDIFrameWindowImpl<MainFrame>)
		CHAIN_MSG_MAP(splitterBase);
	ALT_MSG_MAP(QUICK_SEARCH_MAP)
		MESSAGE_HANDLER(WM_CHAR, onQuickSearchChar)
		MESSAGE_HANDLER(WM_KEYDOWN, onQuickSearchChar)
		MESSAGE_HANDLER(WM_KEYUP, onQuickSearchChar)
		MESSAGE_HANDLER(WM_CTLCOLOREDIT, onQuickSearchColor)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onQuickSearchColor)
		MESSAGE_HANDLER(WM_CTLCOLORLISTBOX, onQuickSearchColor)
		COMMAND_CODE_HANDLER(EN_CHANGE, onQuickSearchEditChange)
	END_MSG_MAP()

	BEGIN_UPDATE_UI_MAP(MainFrame)
		UPDATE_ELEMENT(ID_VIEW_TOOLBAR, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_VIEW_STATUS_BAR, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_VIEW_TRANSFER_VIEW, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_TOGGLE_QSEARCH, UPDUI_MENUPOPUP)
	END_UPDATE_UI_MAP()


	LRESULT onSize(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onHashProgress(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onEndSession(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onGetTTH(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFileSettings(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onMatchAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onLink(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onOpenFileList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onTrayIcon(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnViewTransferView(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGetToolTip(int idCtrl, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onCopyData(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onCloseWindows(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRefreshFileList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);	
	LRESULT onQuickConnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onActivateApp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onWebServerSocket(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onAppCommand(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onAway(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onLimiter(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onUpdate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDisableSounds(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onOpenWindows(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onQuickSearchChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onQuickSearchColor(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onQuickSearchEditChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled);
	LRESULT OnViewQuickSearchBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	static DWORD WINAPI stopper(void* p);
	void UpdateLayout(BOOL bResizeBars = TRUE);
	void parseCommandLine(const tstring& cmdLine);

	LRESULT onWhereAreYou(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		return WMU_WHERE_ARE_YOU;
	}

	LRESULT onTray(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) { 
		bTrayIcon = false;
		updateTray(true); 
		return 0;
	}

	LRESULT onRowsChanged(UINT /*uMsg*/, WPARAM /* wParam */, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		UpdateLayout();
		Invalidate();
		return 0;
	}

	LRESULT onSelected(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		HWND hWnd = (HWND)wParam;
		if(MDIGetActive() != hWnd) {
			MDIActivate(hWnd);
		} else if(BOOLSETTING(TOGGLE_ACTIVE_WINDOW)) {
			::SetWindowPos(hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
			MDINext(hWnd);
			hWnd = MDIGetActive();
		}
		if(::IsIconic(hWnd))
			::ShowWindow(hWnd, SW_RESTORE);
		return 0;
	}
	
	LRESULT onDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	
	LRESULT OnEraseBackground(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		return 0;
	}

	LRESULT OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		PostMessage(WM_CLOSE);
		return 0;
	}
	
	LRESULT onOpenDownloads(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		WinUtil::openFile(Text::toT(SETTING(DOWNLOAD_DIRECTORY)));
		return 0;
	}

	LRESULT OnWindowCascade(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		MDICascade();
		return 0;
	}

	LRESULT OnWindowTile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		MDITile();
		return 0;
	}

	LRESULT OnWindowTileVert(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		MDITile(MDITILE_VERTICAL);
		return 0;
	}

	LRESULT OnWindowArrangeIcons(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		MDIIconArrange();
		return 0;
	}

	LRESULT onWindowMinimizeAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		HWND tmpWnd = GetWindow(GW_CHILD); //getting client window
		tmpWnd = ::GetWindow(tmpWnd, GW_CHILD); //getting first child window
		while (tmpWnd!=NULL) {
			::CloseWindow(tmpWnd);
			tmpWnd = ::GetWindow(tmpWnd, GW_HWNDNEXT);
		}
		return 0;
	}
	LRESULT onWindowRestoreAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		HWND tmpWnd = GetWindow(GW_CHILD); //getting client window
		HWND ClientWnd = tmpWnd; //saving client window handle
		tmpWnd = ::GetWindow(tmpWnd, GW_CHILD); //getting first child window
		BOOL bmax;
		while (tmpWnd!=NULL) {
			::ShowWindow(tmpWnd, SW_RESTORE);
			::SendMessage(ClientWnd,WM_MDIGETACTIVE,NULL,(LPARAM)&bmax);
			if(bmax)break; //bmax will be true if active child 
					//window is maximized, so if bmax then break
			tmpWnd = ::GetWindow(tmpWnd, GW_HWNDNEXT);
		}
		return 0;
	}	

	LRESULT onShutDown(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		setShutDown(!getShutDown());
		return S_OK;
	}
	LRESULT OnCreateToolbar(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		createToolbar();
		return S_OK;
	}
	static MainFrame* getMainFrame() { return anyMF; }
	bool getAppMinimized() const { return bAppMinimized; }
	CToolBarCtrl& getToolBar() { return ctrlToolbar; }

	static void setShutDown(bool b) {
		if (b)
			iCurrentShutdownTime = GET_TICK() / 1000;
		bShutdown = b;
	}
	static bool getShutDown() { return bShutdown; }
	
	static void setAwayButton(bool check) {
		anyMF->ctrlToolbar.CheckButton(IDC_AWAY, check);
	}

	void ShowBalloonTip(tstring szMsg, tstring szTitle, DWORD dwInfoFlags=NIIF_INFO);
	void updateQuickSearches();
	
	CImageList largeImages, largeImagesHot;
	int run();
	

private:
	NOTIFYICONDATA normalicon;
	NOTIFYICONDATA pmicon;

	class DirectoryListInfo {
	public:
		DirectoryListInfo(const HintedUser& aUser, const tstring& aFile, const tstring& aDir, int64_t aSpeed) : user(aUser), file(aFile), dir(aDir), speed(aSpeed) { }
		HintedUser user;
		tstring file;
		tstring dir;
		int64_t speed;
	};
	class DirectoryBrowseInfo {
	public:
		DirectoryBrowseInfo(const HintedUser& ptr, string aText) : user(ptr), text(aText) { }
		HintedUser user;
		string text;
	};
	class FileListQueue: public Thread {
	public:
		bool stop;
		Semaphore s;
		CriticalSection cs;
		list<DirectoryListInfo*> fileLists;

		FileListQueue() : stop(true) {}
		~FileListQueue() throw() {
			shutdown();
		}

		int run();
		void shutdown() {
			stop = true;
			s.signal();
		}
	};
	
	TransferView transferView;
	static MainFrame* anyMF;
	
	enum { MAX_CLIENT_LINES = 10 };
	TStringList lastLinesList;
	tstring lastLines;
	CToolTipCtrl ctrlLastLines;

	CStatusBarCtrl ctrlStatus;
	FlatTabCtrl ctrlTab;
	HttpConnection* c;
	string versionInfo;
	CImageList images;
	CToolBarCtrl ctrlToolbar;

	CToolBarCtrl ctrlQuickSearchBar;
	CComboBox QuickSearchBox;
	CEdit QuickSearchEdit;
	CContainedWindow QuickSearchBoxContainer;
	CContainedWindow QuickSearchEditContainer;
	bool m_bDisableAutoComplete;

	bool tbarcreated;
	bool awaybyminimize;
	bool bTrayIcon;
	bool bAppMinimized;
	bool bIsPM;
	
	static bool bShutdown;
	static uint64_t iCurrentShutdownTime;
	HICON hShutdownIcon;
	static bool isShutdownStatus;

	CMenu trayMenu;

	UINT trayMessage;
	/** Was the window maximized when minimizing it? */
	bool maximized;
	uint64_t lastMove;
	uint64_t lastUpdate;
	int64_t lastUp;
	int64_t lastDown;
	tstring lastTTHdir;
	bool oldshutdown;

	int tabPos;
	bool closing;

	int lastUpload;

	int statusSizes[10];
	
	HANDLE stopperThread;

	FileListQueue listQueue;
	bool missedAutoConnect;
	HWND createToolbar();
	HWND createQuickSearchBar();
	void updateTray(bool add = true);

	LRESULT onAppShow(WORD /*wNotifyCode*/,WORD /*wParam*/, HWND, BOOL& /*bHandled*/) {
		if (::IsIconic(m_hWnd)) {
			ShowWindow(SW_SHOW);
			ShowWindow(maximized ? SW_MAXIMIZE : SW_RESTORE);
		}
		return 0;
	}

	void autoConnect(const FavoriteHubEntry::List& fl);

	MainFrame(const MainFrame&) { dcassert(0); }

	// LogManagerListener
	void on(LogManagerListener::Message, const string& m) throw() { PostMessage(WM_SPEAKER, STATUS_MESSAGE, (LPARAM)new tstring(Text::toT(m))); }

	// TimerManagerListener
	void on(TimerManagerListener::Second, uint64_t aTick) throw();
	
	// HttpConnectionListener
	void on(HttpConnectionListener::Complete, HttpConnection* conn, string const& /*aLine*/, bool /*fromCoral*/) throw();
	void on(HttpConnectionListener::Data, HttpConnection* /*conn*/, const uint8_t* buf, size_t len) throw();	
	// WebServerListener
	void on(WebServerListener::Setup);
	void on(WebServerListener::ShutdownPC, int);

	// QueueManagerListener
	void on(QueueManagerListener::Finished, const QueueItem* qi, const string& dir, const Download*) throw();
	void on(PartialList, const HintedUser&, const string& text) throw();
};

#endif // !defined(MAIN_FRM_H)

/**
 * @file
 * $Id$
 */
