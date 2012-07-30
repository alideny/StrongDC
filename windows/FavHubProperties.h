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

#if !defined(FAV_HUB_PROPERTIES_H)
#define FAV_HUB_PROPERTIES_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <atlcrack.h>

class FavHubProperties : public CDialogImpl<FavHubProperties>
{
public:
	FavHubProperties::FavHubProperties(FavoriteHubEntry *_entry) : entry(_entry) { }
	~FavHubProperties() { }

	enum { IDD = IDD_FAVORITEHUB };
	
	BEGIN_MSG_MAP_EX(FavHubProperties)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_HANDLER(IDC_HUBNICK, EN_CHANGE, OnTextChanged)
		COMMAND_HANDLER(IDC_HUBPASS, EN_CHANGE, OnTextChanged)
		COMMAND_HANDLER(IDC_HUBUSERDESCR, EN_CHANGE, OnTextChanged)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()
	
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnTextChanged(WORD /*wNotifyCode*/, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/);

protected:
	FavoriteHubEntry *entry;
};

#endif // !defined(FAV_HUB_PROPERTIES_H)

/**
 * @file
 * $Id$
 */
