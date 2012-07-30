/* 
* Copyright (C) 2003-2005 Pär Björklund, per.bjorklund@gmail.com
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

#ifndef POPUPWND_H
#define POPUPWND_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"
#include "WinUtil.h"

class PopupWnd : public CWindowImpl<PopupWnd, CWindow>
{
public:
	DECLARE_WND_CLASS(_T("Popup"));

	BEGIN_MSG_MAP(PopupWnd)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		MESSAGE_HANDLER(WM_LBUTTONDOWN, onLButtonDown)
	END_MSG_MAP()

	PopupWnd(const tstring& aMsg, const tstring& aTitle, CRect rc, uint32_t aId): visible(GET_TICK()), id(aId), msg(aMsg), title(aTitle) {
		Create(NULL, rc, NULL, WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_TOOLWINDOW );

		WinUtil::decodeFont(Text::toT(SETTING(TEXT_FONT))/*SETTING(POPUP_FONT)*/, logFont);
		font = ::CreateFontIndirect(&logFont);
	}

	~PopupWnd(){
		DeleteObject(font);
	}

	LRESULT onLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled){
		::PostMessage(WinUtil::mainWnd, WM_SPEAKER, WM_CLOSE, (LPARAM)id);
		bHandled = TRUE;
		return 0;
	}

	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled){
		::SetClassLongPtr(m_hWnd, GCLP_HBRBACKGROUND, (LONG_PTR)::GetSysColorBrush(COLOR_INFOTEXT));
		CRect rc;
		GetClientRect(rc);

		rc.top += 1;
		rc.left += 1;
		rc.right -= 1;
		rc.bottom /= 3;

		label.Create(m_hWnd, rc, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
			SS_CENTER | SS_NOPREFIX);

		rc.top += rc.bottom - 1;
		rc.bottom *= 3;

		label1.Create(m_hWnd, rc, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
			SS_CENTER | SS_NOPREFIX);

		label.SetFont(WinUtil::boldFont);
		label.SetWindowText(title.c_str());
		label1.SetFont(WinUtil::font);
		label1.SetWindowText(msg.c_str());


		bHandled = false;
		return 1;
	}

	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled){
		label.DestroyWindow();
		label.Detach();
		DestroyWindow();

		bHandled = false;
		return 1;
	}

	LRESULT onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		HDC hDC = (HDC)wParam;
				::SetBkColor(hDC, ::GetSysColor(COLOR_INFOBK));
				::SetTextColor(hDC, ::GetSysColor(COLOR_INFOTEXT));
		return (LRESULT)::GetSysColorBrush(COLOR_INFOBK);
	}

	uint32_t id;
	uint64_t visible;

private:
	tstring  msg, title;
	CStatic label, label1;
	LOGFONT logFont;
	HFONT   font;
};


#endif