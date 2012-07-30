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

#if !defined(FINISHED_FRAME_BASE_H)
#define FINISHED_FRAME_BASE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"

#include "FlatTabCtrl.h"
#include "TypedListViewCtrl.h"
#include "ShellContextMenu.h"
#include "WinUtil.h"
#include "TextFrame.h"

#include "../client/ClientManager.h"
#include "../client/StringTokenizer.h"
#include "../client/FinishedManager.h"

template<class T, int title, int id, int icon>
class FinishedFrameBase : public MDITabChildWindowImpl<T, RGB(0, 0, 0), icon>, public StaticFrame<T, title, id>,
	protected FinishedManagerListener, private SettingsManagerListener

{
public:
	typedef MDITabChildWindowImpl<T, RGB(0, 0, 0), icon> baseClass;

	FinishedFrameBase() : totalBytes(0), totalSpeed(0), closed(false) { }
	virtual ~FinishedFrameBase() { }

	BEGIN_MSG_MAP(T)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		COMMAND_ID_HANDLER(IDC_REMOVE, onRemove)
		COMMAND_ID_HANDLER(IDC_TOTAL, onRemove)
		COMMAND_ID_HANDLER(IDC_VIEW_AS_TEXT, onViewAsText)
		COMMAND_ID_HANDLER(IDC_OPEN_FILE, onOpenFile)
		COMMAND_ID_HANDLER(IDC_OPEN_FOLDER, onOpenFolder)
		COMMAND_ID_HANDLER(IDC_GETLIST, onGetList)
		COMMAND_ID_HANDLER(IDC_GRANTSLOT, onGrant)		
		NOTIFY_HANDLER(id, LVN_GETDISPINFO, ctrlList.onGetDispInfo)
		NOTIFY_HANDLER(id, LVN_COLUMNCLICK, ctrlList.onColumnClick)
		NOTIFY_HANDLER(id, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(id, NM_DBLCLK, onDoubleClick)	
		CHAIN_MSG_MAP(baseClass)
	END_MSG_MAP()

	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
		ctrlStatus.Attach(m_hWndStatusBar);

		ctrlList.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
			WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS | LVS_SINGLESEL, WS_EX_CLIENTEDGE, id);
		ctrlList.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

		ctrlList.SetImageList(WinUtil::fileImages, LVSIL_SMALL);
		ctrlList.SetBkColor(WinUtil::bgColor);
		ctrlList.SetTextBkColor(WinUtil::bgColor);
		ctrlList.SetTextColor(WinUtil::textColor);

		// Create listview columns
		WinUtil::splitTokens(columnIndexes, SettingsManager::getInstance()->get(columnOrder), FinishedItem::COLUMN_LAST);
		WinUtil::splitTokens(columnSizes, SettingsManager::getInstance()->get(columnWidth), FinishedItem::COLUMN_LAST);

		for(uint8_t j=0; j<FinishedItem::COLUMN_LAST; j++) {
			int fmt = (j == FinishedItem::COLUMN_SIZE || j == FinishedItem::COLUMN_SPEED) ? LVCFMT_RIGHT : LVCFMT_LEFT;
			ctrlList.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
		}

		ctrlList.SetColumnOrderArray(FinishedItem::COLUMN_LAST, columnIndexes);
		ctrlList.setVisible(SettingsManager::getInstance()->get(columnVisible));
		ctrlList.setSortColumn(FinishedItem::COLUMN_DONE);

		UpdateLayout();

		SettingsManager::getInstance()->addListener(this);
		FinishedManager::getInstance()->addListener(this);
		updateList(FinishedManager::getInstance()->lockList(upload));
		FinishedManager::getInstance()->unlockList();

		ctxMenu.CreatePopupMenu();
		ctxMenu.AppendMenu(MF_STRING, IDC_VIEW_AS_TEXT, CTSTRING(VIEW_AS_TEXT));
		ctxMenu.AppendMenu(MF_STRING, IDC_OPEN_FILE, CTSTRING(OPEN));
		ctxMenu.AppendMenu(MF_STRING, IDC_OPEN_FOLDER, CTSTRING(OPEN_FOLDER));
		ctxMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT, CTSTRING(GRANT_EXTRA_SLOT));
		ctxMenu.AppendMenu(MF_STRING, IDC_GETLIST, CTSTRING(GET_FILE_LIST));
		ctxMenu.AppendMenu(MF_SEPARATOR);
		ctxMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));
		ctxMenu.AppendMenu(MF_STRING, IDC_TOTAL, CTSTRING(REMOVE_ALL));
		ctxMenu.SetMenuDefaultItem(IDC_OPEN_FILE);

		bHandled = FALSE;
		return TRUE;
	}

	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		if(!closed) {
			FinishedManager::getInstance()->removeListener(this);
			SettingsManager::getInstance()->removeListener(this);

			closed = true;
			WinUtil::setButtonPressed(id, false);
			PostMessage(WM_CLOSE);
			return 0;
		} else {
			ctrlList.saveHeaderOrder(columnOrder, columnWidth, columnVisible);
			frame = NULL;
			ctrlList.DeleteAllItems();

			bHandled = FALSE;
			return 0;
		}
	}

	LRESULT onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMITEMACTIVATE * const item = (NMITEMACTIVATE*) pnmh;

		if(item->iItem != -1) {
			FinishedItem *ii = ctrlList.getItemData(item->iItem);
			WinUtil::openFile(Text::toT(ii->getTarget()));
		}
		return 0;
	}

	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
		if(wParam == SPEAK_ADD_LINE) {
			FinishedItem* entry = reinterpret_cast<FinishedItem*>(lParam);
			addEntry(entry);
			if(SettingsManager::getInstance()->get(boldFinished))
				setDirty();
			updateStatus();
		}
		return 0;
	}

	LRESULT onRemove(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		switch(wID)
		{
		case IDC_REMOVE:
			{
				int i = -1;
				while((i = ctrlList.GetNextItem(-1, LVNI_SELECTED)) != -1) {
					FinishedItem *ii = ctrlList.getItemData(i);
					FinishedManager::getInstance()->remove(ii, upload);
					ctrlList.DeleteItem(i);
					
					totalBytes -= ii->getSize();
					totalSpeed -= ii->getAvgSpeed();
					
					delete ii;
				}
				updateStatus();
				break;
			}
		case IDC_TOTAL:
			FinishedManager::getInstance()->removeAll(upload);
			
			ctrlList.DeleteAllItems();
			for(int i = 0; i < ctrlList.GetItemCount(); ++i) {
				delete ctrlList.getItemData(i);
			}
			
			totalBytes = 0;
			totalSpeed = 0;
			updateStatus();
			break;
		}
		return 0;
	}

	LRESULT onViewAsText(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		int i;
		if((i = ctrlList.GetNextItem(-1, LVNI_SELECTED)) != -1) {
			FinishedItem *ii = ctrlList.getItemData(i);
			if(ii != NULL)
				TextFrame::openWindow(Text::toT(ii->getTarget()));
		}
		return 0;
	}

	LRESULT onOpenFile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		int i;
		if((i = ctrlList.GetNextItem(-1, LVNI_SELECTED)) != -1) {
			FinishedItem *ii = ctrlList.getItemData(i);
			if(ii != NULL)
				WinUtil::openFile(Text::toT(ii->getTarget()));
		}
		return 0;
	}

	LRESULT onOpenFolder(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		int i;
		if((i = ctrlList.GetNextItem(-1, LVNI_SELECTED)) != -1) {
			FinishedItem *ii = ctrlList.getItemData(i);
			if(ii != NULL)
				::ShellExecute(NULL, NULL, Text::toT(Util::getFilePath(ii->getTarget())).c_str(), NULL, NULL, SW_SHOWNORMAL);
		}
		return 0;
	}

	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
		if (reinterpret_cast<HWND>(wParam) == ctrlList && ctrlList.GetSelectedCount() > 0) { 
			POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

			if(pt.x == -1 && pt.y == -1) {
				WinUtil::getContextMenuPos(ctrlList, pt);
			}

			bool bShellMenuShown = false;
			if(BOOLSETTING(SHOW_SHELL_MENU) && (ctrlList.GetSelectedCount() == 1) && (LOBYTE(LOWORD(GetVersion())) >= 5)) {
				tstring path = Text::toT(ctrlList.getItemData(ctrlList.GetSelectedIndex())->getTarget());
				if(GetFileAttributes(path.c_str()) != 0xFFFFFFFF) { // Check that the file still exists
					CShellContextMenu shellMenu;
					shellMenu.SetPath(path);

					CMenu* pShellMenu = shellMenu.GetMenu();
					pShellMenu->AppendMenu(MF_STRING, IDC_VIEW_AS_TEXT, CTSTRING(VIEW_AS_TEXT));
					pShellMenu->AppendMenu(MF_STRING, IDC_OPEN_FOLDER, CTSTRING(OPEN_FOLDER));
					pShellMenu->AppendMenu(MF_SEPARATOR);
					pShellMenu->AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));
					pShellMenu->AppendMenu(MF_STRING, IDC_TOTAL, CTSTRING(REMOVE_ALL));
					pShellMenu->AppendMenu(MF_SEPARATOR);

					UINT idCommand = shellMenu.ShowContextMenu(m_hWnd, pt);
					if(idCommand != 0) {
						PostMessage(WM_COMMAND, idCommand);

						bShellMenuShown = true;
					}
				}
			}

			if(!bShellMenuShown)
				ctxMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);			

			return TRUE; 
		}
		bHandled = FALSE;
		return FALSE; 
	}

	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
		NMLVKEYDOWN* kd = reinterpret_cast<NMLVKEYDOWN*>(pnmh);

		if(kd->wVKey == VK_DELETE) {
			PostMessage(WM_COMMAND, IDC_REMOVE);
		} 
		return 0;
	}

	LRESULT onGetList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		int i;
		if((i = ctrlList.GetNextItem(-1, LVNI_SELECTED)) != -1) {
			FinishedItem *ii = ctrlList.getItemData(i);
			if(ii->getUser().user->isOnline()) {
				try {
					QueueManager::getInstance()->addList(ii->getUser(), QueueItem::FLAG_CLIENT_VIEW);
				} catch(const Exception&) {
				}
			} else {
				addStatusLine(TSTRING(USER_OFFLINE));
			}
		}
		return 0;
	}

	LRESULT onGrant(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		int i;
		if((i = ctrlList.GetNextItem(-1, LVNI_SELECTED)) != -1) {
			FinishedItem *ii = ctrlList.getItemData(i);
			if(ii->getUser().user->isOnline()) {
				UploadManager::getInstance()->reserveSlot(ii->getUser(), 600);
			} else {
				addStatusLine(TSTRING(USER_OFFLINE));
			}
		}
		return 0;
	}

	void UpdateLayout(BOOL bResizeBars = TRUE) {
		RECT rect;
		GetClientRect(&rect);

		// position bars and offset their dimensions
		UpdateBarsPosition(rect, bResizeBars);

		if(ctrlStatus.IsWindow()) {
			CRect sr;
			int w[4];
			ctrlStatus.GetClientRect(sr);
			w[3] = sr.right - 16;
			w[2] = max(w[3] - 100, 0);
			w[1] = max(w[2] - 100, 0);
			w[0] = max(w[1] - 100, 0);

			ctrlStatus.SetParts(4, w);
		}

		CRect rc(rect);
		ctrlList.MoveWindow(rc);
	}

	LRESULT onSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /* bHandled */) {
		ctrlList.SetFocus();
		return 0;
	}

protected:
	enum {
		SPEAK_ADD_LINE,
		//SPEAK_REMOVE,
		//SPEAK_REMOVE_ALL
	};

	CStatusBarCtrl ctrlStatus;
	CMenu ctxMenu;

	TypedListViewCtrl<FinishedItem, id> ctrlList;

	int64_t totalBytes;
	int64_t totalSpeed;

	bool closed;

	bool upload;
	SettingsManager::IntSetting boldFinished;
	SettingsManager::StrSetting columnWidth;
	SettingsManager::StrSetting columnOrder;
	SettingsManager::StrSetting columnVisible;
	

	static int columnSizes[FinishedItem::COLUMN_LAST];
	static int columnIndexes[FinishedItem::COLUMN_LAST];

	void addStatusLine(const tstring& aLine) {
		ctrlStatus.SetText(0, (Text::toT(Util::getShortTimeString()) + _T(" ") + aLine).c_str());
	}

	void updateStatus() {
		int count = ctrlList.GetItemCount();
		ctrlStatus.SetText(1, (Util::toStringW(count) + _T(" ") + TSTRING(ITEMS)).c_str());
		ctrlStatus.SetText(2, Util::formatBytesW(totalBytes).c_str());
		ctrlStatus.SetText(3, (Util::formatBytesW(count > 0 ? totalSpeed / count : 0) + _T("/s")).c_str());
	}

	void updateList(const FinishedItemList& fl) {
		ctrlList.SetRedraw(FALSE);
		for(FinishedItemList::const_iterator i = fl.begin(); i != fl.end(); ++i) {
			addEntry(*i);
		}
		ctrlList.SetRedraw(TRUE);
		ctrlList.Invalidate();
		updateStatus();
	}

	void addEntry(FinishedItem* entry) {
		totalBytes += entry->getSize();
		totalSpeed += entry->getAvgSpeed();

		int image = WinUtil::getIconIndex(Text::toT(entry->getTarget()));
		int loc = ctrlList.insertItem(entry, image);
		ctrlList.EnsureVisible(loc, FALSE);
	}

	void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw() {
		bool refresh = false;
		if(ctrlList.GetBkColor() != WinUtil::bgColor) {
			ctrlList.SetBkColor(WinUtil::bgColor);
			ctrlList.SetTextBkColor(WinUtil::bgColor);
			refresh = true;
		}
		if(ctrlList.GetTextColor() != WinUtil::textColor) {
			ctrlList.SetTextColor(WinUtil::textColor);
			refresh = true;
		}
		if(refresh == true) {
			RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
		}
	}

};

template <class T, int title, int id, int icon>
int FinishedFrameBase<T, title, id, icon>::columnIndexes[] = { FinishedItem::COLUMN_DONE, FinishedItem::COLUMN_FILE,
FinishedItem::COLUMN_PATH, FinishedItem::COLUMN_NICK, FinishedItem::COLUMN_HUB, FinishedItem::COLUMN_SIZE, FinishedItem::COLUMN_SPEED };

template <class T, int title, int id, int icon>
int FinishedFrameBase<T, title, id, icon>::columnSizes[] = { 100, 110, 290, 125, 80, 80 };
static ResourceManager::Strings columnNames[] = { ResourceManager::FILENAME, ResourceManager::TIME, ResourceManager::PATH, 
ResourceManager::NICK, ResourceManager::HUB, ResourceManager::SIZE, ResourceManager::SPEED
};

int FinishedItem::getImageIndex() const { return WinUtil::getIconIndex(Text::toT(getTarget())); }

#endif // !defined(FINISHED_FRAME_BASE_H)

/**
* @file
* $Id$
*/
