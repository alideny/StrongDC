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

#if !defined(FLAT_TAB_CTRL_H)
#define FLAT_TAB_CTRL_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../client/SettingsManager.h"
#include "../client/ResourceManager.h"

#include "WinUtil.h"
#include "resource.h"
#include "OMenu.h"

enum {
	FT_FIRST = WM_APP + 700,
	/** This will be sent when the user presses a tab. WPARAM = HWND */
	FTM_SELECTED,
	/** The number of rows changed */
	FTM_ROWS_CHANGED,
	/** Set currently active tab to the HWND pointed by WPARAM */
	FTM_SETACTIVE,
	/** Display context menu and return TRUE, or return FALSE for the default one */
	FTM_CONTEXTMENU,
	/** Close window with postmessage... */
	WM_REALLY_CLOSE
};

template <class T, class TBase = CWindow, class TWinTraits = CControlWinTraits>
class ATL_NO_VTABLE FlatTabCtrlImpl : public CWindowImpl< T, TBase, TWinTraits> {
public:

	enum { FT_EXTRA_SPACE = 18 };

	FlatTabCtrlImpl() : closing(NULL), rows(1), height(0), active(NULL), moving(NULL), inTab(false) { 
		black.CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
	}
	virtual ~FlatTabCtrlImpl() { }

	static LPCTSTR GetWndClassName()
	{
		return _T("FlatTabCtrl");
	}

	void addTab(HWND hWnd, COLORREF color = RGB(0, 0, 0), long icon = 0, long stateIcon = 0) {
		TabInfo* i = new TabInfo(hWnd, color, icon, (stateIcon != 0) ? stateIcon : icon);
		dcassert(getTabInfo(hWnd) == NULL);
		tabs.push_back(i);

		if(SETTING(TABS_POS) == 0 || SETTING(TABS_POS) == 1)
			viewOrder.push_back(hWnd);
		else
			viewOrder.push_front(hWnd);

		nextTab = --viewOrder.end();
		active = i;
		calcRows(false);
		Invalidate();		
	}

	void removeTab(HWND aWnd) {
		TabInfo::List::iterator i;
		for(i = tabs.begin(); i != tabs.end(); ++i) {
			if((*i)->hWnd == aWnd)
				break;
		}

		dcassert(i != tabs.end());
		TabInfo* ti = *i;
		if(active == ti)
			active = NULL;
		if(moving == ti)
			moving = NULL;

		tabs.erase(i);
		viewOrder.remove(aWnd);
		nextTab = viewOrder.end();
		if(!viewOrder.empty())
			--nextTab;

		calcRows(false);
		Invalidate();

		delete ti;
	}

	void startSwitch() {
		if(!viewOrder.empty())
			nextTab = --viewOrder.end();
		inTab = true;
	}

	void endSwitch() {
		inTab = false;
		if(active) 
			setTop(active->hWnd);
	}

	HWND getNext() {
		if(viewOrder.empty())
			return NULL;
		if(nextTab == viewOrder.begin()) {
			nextTab = --viewOrder.end();
		} else {
			--nextTab;
		}
		return *nextTab;
	}
	HWND getPrev() {
		if(viewOrder.empty())
			return NULL;
		nextTab++;
		if(nextTab == viewOrder.end()) {
			nextTab = viewOrder.begin();
		}
		return *nextTab;
	}

	void setActive(HWND aWnd) {
		if(!inTab)
			setTop(aWnd);

		TabInfo* ti = getTabInfo(aWnd);
		
		if(ti == NULL)
			return;
			
		active = ti;
		ti->dirty = false;
		calcRows(false);
		Invalidate();
	}

	void setTop(HWND aWnd) {
		viewOrder.remove(aWnd);
		viewOrder.push_back(aWnd);
		nextTab = --viewOrder.end();
	}

	void setDirty(HWND aWnd) {
		TabInfo* ti = getTabInfo(aWnd);
		dcassert(ti != NULL);
		bool inval = ti->update();
		
		if(active != ti) {
			if(!ti->dirty) {
				ti->dirty = true;
				inval = true;
			}
		}

		if(inval) {
			calcRows(false);
			Invalidate();
		}
	}

	void setIconState(HWND aWnd) {
		TabInfo* ti = getTabInfo(aWnd);
		if(ti != NULL) {
			ti->bState = true;
			Invalidate();
		}
	}
	void unsetIconState(HWND aWnd) {
		TabInfo* ti = getTabInfo(aWnd);
		if(ti != NULL) {
			ti->bState = false;
			Invalidate();
		}
	}

	void setColor(HWND aWnd, COLORREF color) {
		TabInfo* ti = getTabInfo(aWnd);
		if(ti != NULL) {
			ti->pen.DeleteObject();
			ti->pen.CreatePen(PS_SOLID, 1, color);
			Invalidate();
		}
	}

	void updateText(HWND aWnd, LPCTSTR text) {
		TabInfo* ti = getTabInfo(aWnd);
		if(ti != NULL) {
			ti->updateText(text);
			calcRows(false);
			Invalidate();
		}
	}

	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_SIZE, onSize)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_PAINT, onPaint)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, onLButtonDown)
		MESSAGE_HANDLER(WM_LBUTTONUP, onLButtonUp)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_MOUSEMOVE, onMouseMove)		
		MESSAGE_HANDLER(WM_MBUTTONUP, onCloseTab)
		COMMAND_ID_HANDLER(IDC_CLOSE_WINDOW, onCloseWindow)
		COMMAND_ID_HANDLER(IDC_CHEVRON, onChevron)
		COMMAND_RANGE_HANDLER(IDC_SELECT_WINDOW, IDC_SELECT_WINDOW+tabs.size(), onSelectWindow)
	END_MSG_MAP()

	LRESULT onLButtonDown(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
		int xPos = GET_X_LPARAM(lParam); 
		int yPos = GET_Y_LPARAM(lParam); 
		int row = getRows() - ((yPos / getTabHeight()) + 1);

		for(TabInfo::ListIter i = tabs.begin(); i != tabs.end(); ++i) {
			TabInfo* t = *i;
			if((row == t->row) && (xPos >= t->xpos) && (xPos < (t->xpos + t->getWidth())) ) {
				// Bingo, this was clicked
				HWND hWnd = GetParent();
				if(hWnd) {
					if(wParam & MK_SHIFT)
						::SendMessage(t->hWnd, WM_CLOSE, 0, 0);
					else
						moving = t;
				}
				break;
			}
		}
		return 0;
	}

	LRESULT onLButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
		if (moving) {
			int xPos = GET_X_LPARAM(lParam); 
			int yPos = GET_Y_LPARAM(lParam); 
			int row = getRows() - ((yPos / getTabHeight()) + 1);
			
			bool moveLast = true;

			for(TabInfo::ListIter i = tabs.begin(); i != tabs.end(); ++i) {
				TabInfo* t = *i;
				if((row == t->row) && (xPos >= t->xpos) && (xPos < (t->xpos + t->getWidth())) ) {
					// Bingo, this was clicked
					HWND hWnd = GetParent();
					if(hWnd) {
						if(t == moving) 
							::SendMessage(hWnd, FTM_SELECTED, (WPARAM)t->hWnd, 0);
						else {
							//check if the pointer is on the left or right half of the tab
							//to determine where to insert the tab
							moveTabs(t, xPos > (t->xpos + (t->getWidth()/2)));
						}
					}
					moveLast = false;
					break;
				}
			}
			if(moveLast)
				moveTabs(tabs.back(), true);
			moving = NULL;
		}
		return 0;
	}

	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click 

		ScreenToClient(&pt); 
		int xPos = pt.x;
		int row = getRows() - ((pt.y / getTabHeight()) + 1);

		for(TabInfo::ListIter i = tabs.begin(); i != tabs.end(); ++i) {
			TabInfo* t = *i;
			if((row == t->row) && (xPos >= t->xpos) && (xPos < (t->xpos + t->getWidth())) ) {
				// Bingo, this was clicked, check if the owner wants to handle it...
				if(!::SendMessage(t->hWnd, FTM_CONTEXTMENU, 0, lParam)) {
					closing = t->hWnd;
					ClientToScreen(&pt);
					OMenu mnu;
					mnu.CreatePopupMenu();
					mnu.AppendMenu(MF_STRING, IDC_CLOSE_WINDOW, CTSTRING(CLOSE));
					mnu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, m_hWnd);
				}
				break;
			}
		}
		return 0;
	}

	LRESULT onMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click 

		int xPos = pt.x;
		int row = rows - ((pt.y / height) + 1);

		for(TabInfo::ListIter i = tabs.begin(); i != tabs.end(); ++i) {
			TabInfo* t = *i;
			if((row == t->row) && (xPos >= t->xpos) && (xPos < (t->xpos + t->getWidth()))) {
				// Bingo, the mouse was over this one
				int len = ::GetWindowTextLength(t->hWnd) + 1;
				if(len >= TabInfo::MAX_LENGTH) {
					TCHAR* buf = new TCHAR[len];
					::GetWindowText(t->hWnd, buf, len);
					if(buf != current_tip) {
						tab_tip.DelTool(m_hWnd);
						CToolInfo ti(TTF_SUBCLASS, m_hWnd, 0, NULL, buf);
						tab_tip.AddTool(&ti);
						tab_tip.Activate(TRUE);
						current_tip = buf;
					}
					delete[] buf;
					return 1;
				}
			}
		}
		tab_tip.Activate(FALSE);
		current_tip = _T("");
		return 1;
	}

	LRESULT onCloseWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		if(::IsWindow(closing))
			::SendMessage(closing, WM_CLOSE, 0, 0);
		return 0;
	}

	int getTabHeight() const { return height; }
	int getHeight() const { return (getRows() * getTabHeight())+1; }
	int getFill() const { return (getTabHeight() + 1) / 2; }

	int getRows() const 
		{ return rows; }

	void calcRows(bool inval = true) {
		CRect rc;
		GetClientRect(rc);
		int r = 1;
		int w = 0;
		bool notify = false;
		bool needInval = false;

		for(TabInfo::ListIter i = tabs.begin(); i != tabs.end(); ++i) {
			TabInfo* ti = *i;
			if( (r != 0) && ((w + ti->getWidth()) > rc.Width()) ) {
				if(r >= ((SETTING(TABS_POS) == 0 || SETTING(TABS_POS) == 1) ? SETTING(MAX_TAB_ROWS) : (rc.Height() / getTabHeight()))) {
					notify |= (rows != r);
					rows = r;
					r = 0;
					chevron.EnableWindow(TRUE);
				} else {
					r++;
					w = 0;
				}
			}
			ti->xpos = w;
			needInval |= (ti->row != (r-1));
			ti->row = r-1;
			w += ti->getWidth();
		}

		if(r != 0) {
			chevron.EnableWindow(FALSE);
			notify |= (rows != r);
			rows = r;
		}

		if(notify) {
			::SendMessage(GetParent(), FTM_ROWS_CHANGED, 0, 0);
		}
		if(needInval && inval)
			Invalidate();
	}

	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) { 
		chevron.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
			BS_PUSHBUTTON , 0, IDC_CHEVRON);
		chevron.SetWindowText(_T("\u00bb"));

		mnu.CreatePopupMenu();

		tab_tip.Create(m_hWnd, rcDefault, NULL, TTS_ALWAYSTIP | TTS_NOPREFIX);

		CDC dc(::GetDC(m_hWnd));
		HFONT oldfont = dc.SelectFont(WinUtil::font);
		height = WinUtil::getTextHeight(dc) + 2;
		if (height < 17)
			height = 17;
		dc.SelectFont(oldfont);
		::ReleaseDC(m_hWnd, dc);
		
		return 0;
	}

	LRESULT onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) { 
		calcRows();
		SIZE sz = { LOWORD(lParam), HIWORD(lParam) };
		if(SETTING(TABS_POS) == 0 || SETTING(TABS_POS) == 1)
			chevron.MoveWindow(sz.cx-14, 1, 14, getHeight());
		else
			chevron.MoveWindow(0, sz.cy - 15, 150, 15);
		return 0;
	}
		
	LRESULT onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		RECT rc;
		bool drawActive = false;
		RECT crc;
		GetClientRect(&crc);

		if(GetUpdateRect(&rc, FALSE)) {
			CPaintDC dc(m_hWnd);
			HFONT oldfont = dc.SelectFont(WinUtil::font);

			//ATLTRACE("%d, %d\n", rc.left, rc.right);
			for(TabInfo::ListIter i = tabs.begin(); i != tabs.end(); ++i) {
				TabInfo* t = *i;
				
				if(t->row != -1 && t->xpos < rc.right && t->xpos + t->getWidth() >= rc.left ) {
					if(t != active) {
						drawTab(dc, t, t->xpos, t->row);
					} else {
						drawActive = true;
					}
				}
			}
			HPEN oldpen = dc.SelectPen(black);
			for(int r = 0; r < rows; r++) {
				dc.MoveTo(rc.left, r*getTabHeight());
				dc.LineTo(rc.right, r*getTabHeight());
			}

			if(drawActive) {
				dcassert(active);
				drawTab(dc, active, active->xpos, active->row, true);
				dc.SelectPen(active->pen);
				int y = (rows - active->row -1) * getTabHeight();
				dc.MoveTo(active->xpos, y);
				dc.LineTo(active->xpos + active->getWidth(), y);
			}
			dc.SelectPen(oldpen);
			dc.SelectFont(oldfont);
		}
		return 0;
	}

	LRESULT onChevron(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		while(mnu.GetMenuItemCount() > 0) {
			mnu.RemoveMenu(0, MF_BYPOSITION);
		}
		int n = 0;
		CRect rc;
		GetClientRect(&rc);
		CMenuItemInfo mi;
		mi.fMask = MIIM_ID | MIIM_TYPE | MIIM_DATA | MIIM_STATE;
		mi.fType = MFT_STRING | MFT_RADIOCHECK;

		for(TabInfo::ListIter i = tabs.begin(); i != tabs.end(); ++i) {
			TabInfo* ti = *i;
			if(ti->row == -1) {
				mi.dwTypeData = (LPTSTR)ti->name;
				mi.dwItemData = (ULONG_PTR)ti->hWnd;
				mi.fState = MFS_ENABLED | (ti->dirty ? MFS_CHECKED : 0);
				mi.wID = IDC_SELECT_WINDOW + n;
				mnu.InsertMenuItem(n++, TRUE, &mi);
			} 
		}

		POINT pt;
		chevron.GetClientRect(&rc);
		pt.x = rc.right - rc.left;
		pt.y = 0;
		chevron.ClientToScreen(&pt);
		
		mnu.TrackPopupMenu(TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
		return 0;
	}

	LRESULT onSelectWindow(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		CMenuItemInfo mi;
		mi.fMask = MIIM_DATA;
		
		mnu.GetMenuItemInfo(wID, FALSE, &mi);
		HWND hWnd = GetParent();
		if(hWnd) {
			SendMessage(hWnd, FTM_SELECTED, (WPARAM)mi.dwItemData, 0);
		}
		return 0;		
	}

	void SwitchTo(bool next = true){
		TabInfo::ListIter i = tabs.begin();
		for(; i != tabs.end(); ++i){
			if((*i)->hWnd == active->hWnd){
				if(next){
					++i;
					if(i == tabs.end())
						i = tabs.begin();
				} else{
					if(i == tabs.begin())
						i = tabs.end();
					--i;
				}
				setActive((*i)->hWnd);
				::SetWindowPos((*i)->hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
				break;
			}
		}	
	}

	LRESULT onCloseTab(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
		int xPos = GET_X_LPARAM(lParam); 
		int yPos = GET_Y_LPARAM(lParam); 
		int row = getRows() - ((yPos / getTabHeight()) + 1);

		for(TabInfo::ListIter i = tabs.begin(); i != tabs.end(); ++i) {
			TabInfo* t = *i;
			if((row == t->row) && (xPos >= t->xpos) && (xPos < (t->xpos + t->getWidth())) ) {
				// Bingo, this was clicked
				HWND hWnd = GetParent();
				if(hWnd) {
					::SendMessage(t->hWnd, WM_CLOSE, 0, 0);
				}
				break;
			}
		}
		return 0;
	}

	void updateTabs() {
		for(TabInfo::ListIter i = tabs.begin(); i != tabs.end(); ++i) {
			TabInfo* t = *i;
			t->update(true);
		}
	}

private:
	class TabInfo {
	public:

		typedef vector<TabInfo*> List;
		typedef typename List::const_iterator ListIter;

		enum { MAX_LENGTH = 20 };

		TabInfo(HWND aWnd, COLORREF c, long icon, long stateIcon) : hWnd(aWnd), len(0), xpos(0), row(0), dirty(false), hIcon(NULL), hStateIcon(NULL), bState(false) { 
			if (icon != 0)
				hIcon = (HICON)LoadImage((HINSTANCE)::GetWindowLongPtr(aWnd, GWLP_HINSTANCE), MAKEINTRESOURCE(icon), IMAGE_ICON, 16, 16, LR_DEFAULTSIZE);
			if (icon != stateIcon)
				hStateIcon = (HICON)LoadImage((HINSTANCE)::GetWindowLongPtr(aWnd, GWLP_HINSTANCE), MAKEINTRESOURCE(stateIcon), IMAGE_ICON, 16, 16, LR_DEFAULTSIZE);
			pen.CreatePen(PS_SOLID, 1, c);
			memzero(&size, sizeof(size));
			memzero(&boldSize, sizeof(boldSize));
			name[0] = 0;
			update();
		}

		~TabInfo() {
			DeleteObject(pen);
			if (hIcon == hStateIcon)
				DestroyIcon(hIcon);
			else {
				DestroyIcon(hIcon);
				DestroyIcon(hStateIcon);
			}
		}
	
		HWND hWnd;
		CPen pen;
		TCHAR name[MAX_LENGTH];
		size_t len;
		SIZE size;
		SIZE boldSize;
		int xpos;
		int row;
		bool dirty;
		HICON hIcon;
		HICON hStateIcon;
		bool bState;

		bool update(bool always = false) {
			TCHAR name2[MAX_LENGTH];
			len = (size_t)::GetWindowTextLength(hWnd);
			if(len >= MAX_LENGTH) {
				::GetWindowText(hWnd, name2, MAX_LENGTH - 3);
				name2[MAX_LENGTH - 4] = _T('.');
				name2[MAX_LENGTH - 3] = _T('.');
				name2[MAX_LENGTH - 2] = _T('.');
				name2[MAX_LENGTH - 1] = 0;
				len = MAX_LENGTH - 1;
			} else {
				::GetWindowText(hWnd, name2, MAX_LENGTH);
			}
			if(!always && _tcscmp(name, name2) == 0) {
				return false;
			}
			_tcscpy(name, name2);

			if(SETTING(TABS_POS) == 0 || SETTING(TABS_POS) == 1) {
				CDC dc(::GetDC(hWnd));
				HFONT f = dc.SelectFont(WinUtil::font);
				dc.GetTextExtent(name, len, &size);
				dc.SelectFont(WinUtil::boldFont);
				dc.GetTextExtent(name, len, &boldSize);
				dc.SelectFont(f);		
				::ReleaseDC(hWnd, dc);
			} else {
				size.cx = 150;
				boldSize.cx = 150;
			}
			return true;
		}

		bool updateText(const LPCTSTR text) {
			len = _tcslen(text);
			if(len >= MAX_LENGTH) {
				::_tcsncpy(name, text, MAX_LENGTH - 3);
				name[MAX_LENGTH - 4] = '.';
				name[MAX_LENGTH - 3] = '.';
				name[MAX_LENGTH - 2] = '.';
				name[MAX_LENGTH - 1] = 0;
				len = MAX_LENGTH - 1;
			} else {
				_tcscpy(name, text);
			}

			if(SETTING(TABS_POS) == 0 || SETTING(TABS_POS) == 1) {
				CDC dc(::GetDC(hWnd));
				HFONT f = dc.SelectFont(WinUtil::font);
				dc.GetTextExtent(name, len, &size);
				dc.SelectFont(WinUtil::boldFont);
				dc.GetTextExtent(name, len, &boldSize);
				dc.SelectFont(f);		
				::ReleaseDC(hWnd, dc);
			} else {
				size.cx = 150;
				boldSize.cx = 150;
			}
			return true;
		}

		int getWidth() const {
			return (dirty ? boldSize.cx : size.cx) + FT_EXTRA_SPACE + (hIcon != NULL ? 10 : 0);
		}
	};

	void moveTabs(TabInfo* aTab, bool after) {
		if(moving == NULL)
			return;

		TabInfo::List::iterator i, j;
		//remove the tab we're moving
		for(j = tabs.begin(); j != tabs.end(); ++j) {
			if((*j) == moving) {
				tabs.erase(j);
				break;
			}
		}

		//find the tab we're going to insert before or after
		for(i = tabs.begin(); i != tabs.end(); ++i) {
			if((*i) == aTab) {
				if(after)
					++i;
				break;
			}
		}

		tabs.insert(i, moving);
		moving = NULL;

		calcRows(false);
		Invalidate();	
	}

	HWND closing;
	CButton chevron;
	CMenu mnu;
	CToolTipCtrl tab_tip;
	
	int rows;
	int height;

	tstring current_tip;

	TabInfo* active;
	TabInfo* moving;
	typename TabInfo::List tabs;
	CPen black;

	typedef list<HWND> WindowList;
	typedef WindowList::const_iterator WindowIter;

	WindowList viewOrder;
	WindowIter nextTab;

	bool inTab;

	TabInfo* getTabInfo(HWND aWnd) const {
		for(TabInfo::ListIter i	= tabs.begin(); i != tabs.end(); ++i) {
			if((*i)->hWnd == aWnd)
				return *i;
		}
		return NULL;
	}

	/**
	 * Draws a tab
	 * @return The width of the tab
	 */
	void drawTab(CDC& dc, const TabInfo* tab, int pos, int row, bool aActive = false) {
		
		int ypos = (getRows() - row - 1) * getTabHeight();

		HPEN oldpen = dc.SelectPen(black);
		
		POINT p[4];
		dc.BeginPath();
		dc.MoveTo(pos, ypos);
		p[0].x = pos + tab->getWidth() - ((SETTING(TABS_POS) == 2 || SETTING(TABS_POS) == 3) ? 29 : 0);
		p[0].y = ypos;
		p[1].x = pos + tab->getWidth() - ((SETTING(TABS_POS) == 2 || SETTING(TABS_POS) == 3) ? 29 : 0);
		p[1].y = ypos + getTabHeight();
		p[2].x = pos;
		p[2].y = ypos + getTabHeight();
		p[3].x = pos;
		p[3].y = ypos;
		
		dc.PolylineTo(p, 4);
		dc.CloseFigure();
		dc.EndPath();
		
		HBRUSH hBr = GetSysColorBrush(aActive ? COLOR_WINDOW : COLOR_BTNFACE);
		HBRUSH oldbrush = dc.SelectBrush(hBr);
		dc.FillPath();
		
		dc.MoveTo(p[1].x + 1, p[1].y);
		dc.LineTo(p[0].x + 1, p[0].y);
		dc.MoveTo(p[2]);
		dc.LineTo(p[3]);
		if(!active || (tab->row != (rows - 1)) )
			dc.LineTo(p[0]);
		
		dc.SelectPen(tab->pen);
		dc.MoveTo(p[1]);
		dc.LineTo(p[0]);
		dc.MoveTo(p[1]);
		dc.LineTo(p[2]);
		
		dc.SelectPen(oldpen);
		dc.SelectBrush(oldbrush);
		
		dc.SetBkMode(TRANSPARENT);

		pos = pos + getFill() / 2 + FT_EXTRA_SPACE / 2;
		if (tab->hIcon != 0)
			pos += 10;

		COLORREF oldclr = dc.SetTextColor(GetSysColor(COLOR_BTNTEXT));
		if(tab->dirty) {
			HFONT f = dc.SelectFont(WinUtil::boldFont);
			dc.TextOut(pos, ypos + 2, tab->name, tab->len);
			dc.SelectFont(f);		
		} else {
			dc.TextOut(pos, ypos + 2, tab->name, tab->len);
		}
		if (tab->hIcon != 0) {
			DrawIconEx(dc.m_hDC, pos - 18, ypos + 1, tab->bState ? tab->hStateIcon : tab->hIcon, 16, 16, NULL, hBr, DI_NORMAL | DI_COMPAT);
		}
		dc.SetTextColor(oldclr);
	}
};

class FlatTabCtrl : public FlatTabCtrlImpl<FlatTabCtrl> {
public:
	DECLARE_FRAME_WND_CLASS_EX(GetWndClassName(), IDR_FLATTAB, 0, COLOR_3DFACE);
};

template <class T, int C = RGB(128, 128, 128), long I = 0, long I_state = 0, class TBase = CMDIWindow, class TWinTraits = CMDIChildWinTraits>
class ATL_NO_VTABLE MDITabChildWindowImpl : public CMDIChildWindowImpl<T, TBase, TWinTraits> {
public:

	MDITabChildWindowImpl() : created(false) { }
	FlatTabCtrl* getTab() { return WinUtil::tabCtrl; }

	virtual void OnFinalMessage(HWND /*hWnd*/) { delete this; }
	
 	typedef MDITabChildWindowImpl<T, C, I, I_state, TBase, TWinTraits> thisClass;
	typedef CMDIChildWindowImpl<T, TBase, TWinTraits> baseClass;
	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_SYSCOMMAND, onSysCommand)
		MESSAGE_HANDLER(WM_FORWARDMSG, onForwardMsg)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_MDIACTIVATE, onMDIActivate)
		MESSAGE_HANDLER(WM_DESTROY, onDestroy)
		MESSAGE_HANDLER(WM_SETTEXT, onSetText)
		MESSAGE_HANDLER(WM_REALLY_CLOSE, onReallyClose)
		MESSAGE_HANDLER(WM_NOTIFYFORMAT, onNotifyFormat)
		MESSAGE_HANDLER_HWND(WM_INITMENUPOPUP, OMenu::onInitMenuPopup)
		MESSAGE_HANDLER_HWND(WM_MEASUREITEM, OMenu::onMeasureItem)
		MESSAGE_HANDLER_HWND(WM_DRAWITEM, OMenu::onDrawItem)
		CHAIN_MSG_MAP(baseClass)
	END_MSG_MAP()
	
	HWND Create(HWND hWndParent, ATL::_U_RECT rect = NULL, LPCTSTR szWindowName = NULL,
	DWORD dwStyle = 0, DWORD dwExStyle = 0,
	UINT nMenuID = 0, LPVOID lpCreateParam = NULL)
	{
		ATOM atom = T::GetWndClassInfo().Register(&m_pfnSuperWindowProc);

		if(nMenuID != 0)
#if (_ATL_VER >= 0x0700)
			m_hMenu = ::LoadMenu(ATL::_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCE(nMenuID));
#else //!(_ATL_VER >= 0x0700)
			m_hMenu = ::LoadMenu(_Module.GetResourceInstance(), MAKEINTRESOURCE(nMenuID));
#endif //!(_ATL_VER >= 0x0700)

		if(m_hMenu == NULL)
			m_hMenu = WinUtil::mainMenu;

		dwStyle = T::GetWndStyle(dwStyle);
		dwExStyle = T::GetWndExStyle(dwExStyle);

		dwExStyle |= WS_EX_MDICHILD;	// force this one
		m_pfnSuperWindowProc = ::DefMDIChildProc;
		m_hWndMDIClient = hWndParent;
		ATLASSERT(::IsWindow(m_hWndMDIClient));

		if(rect.m_lpRect == NULL)
			rect.m_lpRect = &TBase::rcDefault;

		// If the currently active MDI child is maximized, we want to create this one maximized too
		ATL::CWindow wndParent = hWndParent;

		wndParent.SetRedraw(FALSE);

		HWND hWnd = CFrameWindowImplBase<TBase, TWinTraits >::Create(hWndParent, rect.m_lpRect, szWindowName, dwStyle, dwExStyle, (UINT)0U, atom, lpCreateParam);

		// Maximize and redraw everything
		if(hWnd != NULL)
			MDIMaximize(hWnd);
		wndParent.SetRedraw(TRUE);
		wndParent.RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
		::SetFocus(GetMDIFrame());   // focus will be set back to this window

		return hWnd;
	}

	LRESULT onNotifyFormat(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
#ifdef _UNICODE
		return NFR_UNICODE;
#else
		return NFR_ANSI;
#endif		
	}

	// All MDI windows must have this in wtl it seems to handle ctrl-tab and so on...
	LRESULT onForwardMsg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
		return baseClass::PreTranslateMessage((LPMSG)lParam);
	}

	LRESULT onSysCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		if(wParam == SC_NEXTWINDOW) {
			HWND next = getTab()->getNext();
			if(next != NULL) {
				MDIActivate(next);
				return 0;
			}
		} else if(wParam == SC_PREVWINDOW) {
			HWND next = getTab()->getPrev();
			if(next != NULL) {
				MDIActivate(next);
				return 0;
			}
		}
		bHandled = FALSE;
		return 0;
	}

	LRESULT onCreate(UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		bHandled = FALSE;
		dcassert(getTab());
		getTab()->addTab(m_hWnd, C, I, I_state);
		created = true;
		return 0;
	}
	
	LRESULT onMDIActivate(UINT /*uMsg*/, WPARAM /*wParam */, LPARAM lParam, BOOL& bHandled) {
		dcassert(getTab());
		if((m_hWnd == (HWND)lParam))
			getTab()->setActive(m_hWnd);

		bHandled = FALSE;
		return 1; 
	}

	LRESULT onDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		bHandled = FALSE;
		dcassert(getTab());
		getTab()->removeTab(m_hWnd);
		if(m_hMenu == WinUtil::mainMenu)
			m_hMenu = NULL;

		return 0;
	}

	LRESULT onReallyClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		MDIDestroy(m_hWnd);
		return 0;
	}

	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled */) {
		PostMessage(WM_REALLY_CLOSE);
		return 0;
	}

	LRESULT onSetText(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
		bHandled = FALSE;
		dcassert(getTab());
		if(created) {
			getTab()->updateText(m_hWnd, (LPCTSTR)lParam);
		}
		return 0;
	}

	LRESULT onKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
		if(wParam == VK_CONTROL && LOWORD(lParam) == 1) {
			getTab()->startSwitch();
		}
		bHandled = FALSE;
		return 0;
	}

	LRESULT onKeyUp(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
		if(wParam == VK_CONTROL) {
			getTab()->endSwitch();
		}
		bHandled = FALSE;
		return 0;
		
	}

	void setDirty() {
		dcassert(getTab());
		getTab()->setDirty(m_hWnd);
	}
	void setTabColor(COLORREF color) {
		dcassert(getTab());
		getTab()->setColor(m_hWnd, color);
	}
	void setIconState() {
		dcassert(getTab());
		getTab()->setIconState(m_hWnd);
	}
	void unsetIconState() {
		dcassert(getTab());
		getTab()->unsetIconState(m_hWnd);
	}
private:
	bool created;
};

#endif // !defined(FLAT_TAB_CTRL_H)

/**
 * @file
 * $Id$
 */
