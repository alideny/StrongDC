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

#if !defined(QUEUE_FRAME_H)
#define QUEUE_FRAME_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "FlatTabCtrl.h"
#include "TypedListViewCtrl.h"

#include "../client/QueueManager.h"
#include "../client/FastAlloc.h"
#include "../client/TaskQueue.h"

#define SHOWTREE_MESSAGE_MAP 12

class QueueFrame : public MDITabChildWindowImpl<QueueFrame, RGB(0, 0, 0), IDR_QUEUE>, public StaticFrame<QueueFrame, ResourceManager::DOWNLOAD_QUEUE, IDC_QUEUE>,
	private QueueManagerListener, public CSplitterImpl<QueueFrame>, private SettingsManagerListener
{
public:
	DECLARE_FRAME_WND_CLASS_EX(_T("QueueFrame"), IDR_QUEUE, 0, COLOR_3DFACE);

	QueueFrame() : menuItems(0), queueSize(0), queueItems(0), spoken(false), dirty(false), 
		usingDirMenu(false),  readdItems(0), fileLists(NULL), showTree(true), closed(false), PreviewAppsSize(0),
		showTreeContainer(WC_BUTTON, this, SHOWTREE_MESSAGE_MAP) 
	{
	}

	~QueueFrame() {
		// Clear up dynamicly allocated menu objects
		browseMenu.ClearMenu();
		removeMenu.ClearMenu();
		removeAllMenu.ClearMenu();
		pmMenu.ClearMenu();
		readdMenu.ClearMenu();		
	}
	
	typedef MDITabChildWindowImpl<QueueFrame, RGB(0, 0, 0), IDR_QUEUE> baseClass;
	typedef CSplitterImpl<QueueFrame> splitBase;

	BEGIN_MSG_MAP(QueueFrame)
		NOTIFY_HANDLER(IDC_QUEUE, LVN_GETDISPINFO, ctrlQueue.onGetDispInfo)
		NOTIFY_HANDLER(IDC_QUEUE, LVN_COLUMNCLICK, ctrlQueue.onColumnClick)
		NOTIFY_HANDLER(IDC_QUEUE, LVN_GETINFOTIP, ctrlQueue.onInfoTip)
		NOTIFY_HANDLER(IDC_QUEUE, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_QUEUE, LVN_ITEMCHANGED, onItemChangedQueue)
		NOTIFY_HANDLER(IDC_DIRECTORIES, TVN_SELCHANGED, onItemChanged)
		NOTIFY_HANDLER(IDC_DIRECTORIES, TVN_KEYDOWN, onKeyDownDirs)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
		COMMAND_ID_HANDLER(IDC_SEARCH_ALTERNATES, onSearchAlternates)
		COMMAND_ID_HANDLER(IDC_COPY_LINK, onCopyMagnet)
		COMMAND_ID_HANDLER(IDC_REMOVE, onRemove)
		COMMAND_ID_HANDLER(IDC_RECHECK, onRecheck);
		COMMAND_ID_HANDLER(IDC_REMOVE_OFFLINE, onRemoveOffline)
		COMMAND_ID_HANDLER(IDC_MOVE, onMove)
		COMMAND_RANGE_HANDLER(IDC_COPY, IDC_COPY + COLUMN_LAST-1, onCopy)
		COMMAND_RANGE_HANDLER(IDC_PRIORITY_PAUSED, IDC_PRIORITY_HIGHEST, onPriority)
		COMMAND_RANGE_HANDLER(IDC_SEGMENTONE, IDC_SEGMENTTEN, onSegments)
		COMMAND_RANGE_HANDLER(IDC_BROWSELIST, IDC_BROWSELIST + menuItems, onBrowseList)
		COMMAND_RANGE_HANDLER(IDC_REMOVE_SOURCE, IDC_REMOVE_SOURCE + menuItems, onRemoveSource)
		COMMAND_RANGE_HANDLER(IDC_REMOVE_SOURCES, IDC_REMOVE_SOURCES + 1 + menuItems, onRemoveSources)
		COMMAND_RANGE_HANDLER(IDC_PM, IDC_PM + menuItems, onPM)
		COMMAND_RANGE_HANDLER(IDC_READD, IDC_READD + 1 + readdItems, onReadd)
		COMMAND_ID_HANDLER(IDC_AUTOPRIORITY, onAutoPriority)
		COMMAND_RANGE_HANDLER(IDC_PREVIEW_APP, IDC_PREVIEW_APP + PreviewAppsSize, onPreviewCommand)
		NOTIFY_HANDLER(IDC_QUEUE, NM_CUSTOMDRAW, onCustomDraw)
		CHAIN_MSG_MAP(splitBase)
		CHAIN_MSG_MAP(baseClass)
	ALT_MSG_MAP(SHOWTREE_MESSAGE_MAP)
		MESSAGE_HANDLER(BM_SETCHECK, onShowTree)
	END_MSG_MAP()

	LRESULT onPriority(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSegments(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onBrowseList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRemoveSource(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRemoveSources(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onPM(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onReadd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRecheck(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onSearchAlternates(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopyMagnet(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onAutoPriority(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onPreviewCommand(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onRemoveOffline(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void UpdateLayout(BOOL bResizeBars = TRUE);
	void removeDir(HTREEITEM ht);
	void setPriority(HTREEITEM ht, const QueueItem::Priority& p);
	void setAutoPriority(HTREEITEM ht, const bool& ap);
	void changePriority(bool inc);

	LRESULT onItemChangedQueue(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLISTVIEW* lv = (NMLISTVIEW*)pnmh;
		if((lv->uNewState & LVIS_SELECTED) != (lv->uOldState & LVIS_SELECTED))
			updateStatus();
		return 0;
	}

	LRESULT onSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /* bHandled */) {
		ctrlQueue.SetFocus();
		return 0;
	}

	LRESULT onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		usingDirMenu ? removeSelectedDir() : removeSelected();
		return 0;
	}

	LRESULT onMove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		usingDirMenu ? moveSelectedDir() : moveSelected();
		return 0;
	}

	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
		if(kd->wVKey == VK_DELETE) {
			removeSelected();
		} else if(kd->wVKey == VK_ADD){
			// Increase Item priority
			changePriority(true);
		} else if(kd->wVKey == VK_SUBTRACT){
			// Decrease item priority
			changePriority(false);
		} else if(kd->wVKey == VK_TAB) {
			onTab();
		}
		return 0;
	}

	LRESULT onKeyDownDirs(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMTVKEYDOWN* kd = (NMTVKEYDOWN*) pnmh;
		if(kd->wVKey == VK_DELETE) {
			removeSelectedDir();
		} else if(kd->wVKey == VK_TAB) {
			onTab();
		}
		return 0;
	}

	void onTab();

	LRESULT onShowTree(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		bHandled = FALSE;
		showTree = (wParam == BST_CHECKED);
		UpdateLayout(FALSE);
		return 0;
	}
	
private:

	enum {
		COLUMN_FIRST,
		COLUMN_TARGET = COLUMN_FIRST,
		COLUMN_STATUS,
		COLUMN_SEGMENTS,
		COLUMN_SIZE,
		COLUMN_PROGRESS,
		COLUMN_DOWNLOADED,
		COLUMN_PRIORITY,
		COLUMN_USERS,
		COLUMN_PATH,
		COLUMN_EXACT_SIZE,
		COLUMN_ERRORS,
		COLUMN_ADDED,
		COLUMN_TTH,
		COLUMN_LAST
	};
	enum Tasks {
		ADD_ITEM,
		REMOVE_ITEM,
		UPDATE_ITEM,
		UPDATE_STATUS
	};
	
	class QueueItemInfo;
	friend class QueueItemInfo;
	
	class QueueItemInfo : public FastAlloc<QueueItemInfo> {
	public:

		QueueItemInfo(QueueItem* aQI) : qi(aQI)	{
			qi->inc();
		}

		~QueueItemInfo() { 
			qi->dec();
		}

		void remove() { QueueManager::getInstance()->remove(getTarget()); }

		// TypedListViewCtrl functions
		const tstring getText(int col) const;

		static int compareItems(const QueueItemInfo* a, const QueueItemInfo* b, int col) {
			switch(col) {
				case COLUMN_SIZE: case COLUMN_EXACT_SIZE: return compare(a->getSize(), b->getSize());
				case COLUMN_PRIORITY: return compare((int)a->getPriority(), (int)b->getPriority());
				case COLUMN_DOWNLOADED: return compare(a->getDownloadedBytes(), b->getDownloadedBytes());
				case COLUMN_ADDED: return compare(a->getAdded(), b->getAdded());
				default: return lstrcmpi(a->getText(col).c_str(), b->getText(col).c_str());
			}
		}
		int getImageIndex() const { return WinUtil::getIconIndex(Text::toT(getTarget()));	}

		const QueueItem* getQueueItem() const { return qi; }
		string getPath() const { return Util::getFilePath(getTarget()); }

		bool isSet(Flags::MaskType aFlag) const { return (qi->getFlags() & aFlag) == aFlag; }

		const string& getTarget() const { return qi->getTarget(); }

		int64_t getSize() const { return qi->getSize(); }
		int64_t getDownloadedBytes() const { return qi->getDownloadedBytes(); }

		time_t getAdded() const { return qi->getAdded(); }
		const TTHValue& getTTH() const { return qi->getTTH(); }

		QueueItem::Priority getPriority() const { return qi->getPriority(); }
		bool isWaiting() const { return qi->isWaiting(); }
		bool isFinished() const { return qi->isFinished(); }

		bool getAutoPriority() const { return qi->getAutoPriority(); }

	private:
		QueueItem* qi;

		QueueItemInfo(const QueueItemInfo&);
		QueueItemInfo& operator=(const QueueItemInfo&);
	};
		
	struct QueueItemInfoTask : FastAlloc<QueueItemInfoTask>, public Task {
		QueueItemInfoTask(QueueItemInfo* ii_) : ii(ii_) { }
		QueueItemInfo* ii;
	};

	struct UpdateTask : FastAlloc<UpdateTask>, public Task {
		UpdateTask(const QueueItem& source) : target(source.getTarget()) { }
		string target;
	};

	TaskQueue tasks;
	bool spoken;

	OMenu browseMenu;
	OMenu removeMenu;
	OMenu removeAllMenu;
	OMenu pmMenu;
	OMenu readdMenu;

	int PreviewAppsSize;

	CButton ctrlShowTree;
	CContainedWindow showTreeContainer;
	bool showTree;

	bool usingDirMenu;

	bool dirty;

	int menuItems;
	int readdItems;

	HTREEITEM fileLists;

	typedef unordered_multimap<string, QueueItemInfo*, noCaseStringHash, noCaseStringEq> DirectoryMap;
	typedef DirectoryMap::iterator DirectoryIter;
	typedef DirectoryMap::const_iterator DirectoryIterC;
	typedef pair<DirectoryIterC, DirectoryIterC> DirectoryPairC;
	DirectoryMap directories;
	string curDir;

	TypedListViewCtrl<QueueItemInfo, IDC_QUEUE> ctrlQueue;
	CTreeViewCtrl ctrlDirs;
	
	CStatusBarCtrl ctrlStatus;
	int statusSizes[6];
	
	int64_t queueSize;
	int queueItems;

	bool closed;
	
	static int columnIndexes[COLUMN_LAST];
	static int columnSizes[COLUMN_LAST];

	void addQueueList(const QueueItem::StringMap& l);
	void addQueueItem(QueueItemInfo* qi, bool noSort);
	HTREEITEM addDirectory(const string& dir, bool isFileList = false, HTREEITEM startAt = NULL);
	void removeDirectory(const string& dir, bool isFileList = false);
	void removeDirectories(HTREEITEM ht);

	void updateQueue();
	void updateStatus();
	
	/**
	 * This one is different from the others because when a lot of files are removed
	 * at the same time, the WM_SPEAKER messages seem to get lost in the handling or
	 * something, they're not correctly processed anyway...thanks windows.
	 */
	void speak(Tasks t, Task* p) {
        tasks.add(static_cast<uint8_t>(t), p);
		if(!spoken) {
			spoken = true;
			PostMessage(WM_SPEAKER);
		}
	}

	bool isCurDir(const string& aDir) const { return stricmp(curDir, aDir) == 0; }

	void moveSelected();	
	void moveSelectedDir();
	void moveDir(HTREEITEM ht, const string& target);

	void moveNode(HTREEITEM item, HTREEITEM parent);

	// temporary vector for moving directories
	vector<pair<QueueItemInfo*, string>> tmp;

	void clearTree(HTREEITEM item);

	QueueItemInfo* getItemInfo(const string& target) const;

	void removeSelected();
	void removeSelectedDir();
	
	const string& getSelectedDir() const {
		HTREEITEM ht = ctrlDirs.GetSelectedItem();
		return ht == NULL ? Util::emptyString : getDir(ctrlDirs.GetSelectedItem());
	}
	
	const string& getDir(HTREEITEM ht) const { dcassert(ht != NULL); return *reinterpret_cast<string*>(ctrlDirs.GetItemData(ht)); }

	void on(QueueManagerListener::Added, QueueItem* aQI) throw();
	void on(QueueManagerListener::Moved, const QueueItem* aQI, const string& oldTarget) throw();
	void on(QueueManagerListener::Removed, const QueueItem* aQI) throw();
	void on(QueueManagerListener::SourcesUpdated, const QueueItem* aQI) throw();
	void on(QueueManagerListener::StatusUpdated, const QueueItem* aQI) throw() { on(QueueManagerListener::SourcesUpdated(), aQI); }
	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw();
	
	void onRechecked(const string& target, const string& message);
	
	void on(QueueManagerListener::RecheckStarted, const string& target) throw();
	void on(QueueManagerListener::RecheckNoFile, const string& target) throw();
	void on(QueueManagerListener::RecheckFileTooSmall, const string& target) throw();
	void on(QueueManagerListener::RecheckDownloadsRunning, const string& target) throw();
	void on(QueueManagerListener::RecheckNoTree, const string& target) throw();
	void on(QueueManagerListener::RecheckAlreadyFinished, const string& target) throw();
	void on(QueueManagerListener::RecheckDone, const string& target) throw();
};

#endif // !defined(QUEUE_FRAME_H)

/**
 * @file
 * $Id$
 */
