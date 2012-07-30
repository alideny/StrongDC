/* 
* Copyright (C) 2001-2003 Jacek Sieka, j_s@telia.com
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

#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"
#include "MagnetDlg.h"
#include "WinUtil.h"

LRESULT MagnetDlg::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	// zombies.
	SetWindowText(CTSTRING(MAGNET_DLG_TITLE));
	CenterWindow(GetParent());

	// fill in dialog bits
	SetDlgItemText(IDC_MAGNET_HASH, CTSTRING(MAGNET_DLG_HASH));
	SetDlgItemText(IDC_MAGNET_NAME, CTSTRING(MAGNET_DLG_FILE));
	SetDlgItemText(IDC_MAGNET_SIZE, CTSTRING(MAGNET_DLG_SIZE));
	SetDlgItemText(IDC_MAGNET_QUEUE, CTSTRING(MAGNET_DLG_QUEUE));
	SetDlgItemText(IDC_MAGNET_SEARCH, CTSTRING(MAGNET_DLG_SEARCH));
	SetDlgItemText(IDC_MAGNET_NOTHING, CTSTRING(MAGNET_DLG_NOTHING));
	SetDlgItemText(IDC_MAGNET_REMEMBER, CTSTRING(MAGNET_DLG_REMEMBER));
	if(mSize <= 0 || mFileName.length() <= 0) {
		::ShowWindow(GetDlgItem(IDC_MAGNET_QUEUE), false);
		::ShowWindow(GetDlgItem(IDC_MAGNET_REMEMBER), false);
	}
	SetDlgItemText(IDC_MAGNET_TEXT, CTSTRING(MAGNET_DLG_TEXT_GOOD));

	// file details
	SetDlgItemText(IDC_MAGNET_DISP_HASH, mHash.c_str());

	// handling UTF8 input text
	{
	string strFileName = Text::wideToAcp(mFileName);
	if (Text::validateUtf8(strFileName))
		mFileName = Text::toT(strFileName);
	}

	SetDlgItemText(IDC_MAGNET_DISP_NAME, mFileName.length() > 0 ? mFileName.c_str() : _T("N/A"));
	char buf[32];
	SetDlgItemText(IDC_MAGNET_DISP_SIZE, mSize > 0 ? Text::toT(_i64toa(mSize, buf, 10)).c_str() : _T("N/A"));
		//search->minFileSize > 0 ? _i64toa(search->minFileSize, buf, 10) : ""

	// radio button
	CheckRadioButton(IDC_MAGNET_QUEUE, IDC_MAGNET_NOTHING, IDC_MAGNET_SEARCH);

	// focus
	CEdit focusThis;
	focusThis.Attach(GetDlgItem(IDC_MAGNET_SEARCH));
	focusThis.SetFocus();

	return 0;
}

LRESULT MagnetDlg::onCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(wID == IDOK) {
		if(IsDlgButtonChecked(IDC_MAGNET_REMEMBER) == BST_CHECKED) {
			SettingsManager::getInstance()->set(SettingsManager::MAGNET_ASK,  false);
			if(IsDlgButtonChecked(IDC_MAGNET_QUEUE))
				SettingsManager::getInstance()->set(SettingsManager::MAGNET_ACTION, SettingsManager::MAGNET_AUTO_DOWNLOAD);
			else if(IsDlgButtonChecked(IDC_MAGNET_SEARCH))
				SettingsManager::getInstance()->set(SettingsManager::MAGNET_ACTION, SettingsManager::MAGNET_AUTO_SEARCH);
		}

		if(IsDlgButtonChecked(IDC_MAGNET_SEARCH)) {
			TTHValue tmphash(Text::fromT(mHash));
			WinUtil::searchHash(tmphash); 
		} else if(IsDlgButtonChecked(IDC_MAGNET_QUEUE)) {
			try {
				string target = SETTING(DOWNLOAD_DIRECTORY) + Text::fromT(mFileName);
				QueueManager::getInstance()->add(target, mSize, TTHValue(Text::fromT(mHash)), HintedUser(UserPtr(), Util::emptyString));
			} catch(const Exception& e) {
				LogManager::getInstance()->message(e.getError());
			}
		} 
	}
	EndDialog(wID);
	return 0;
}

LRESULT MagnetDlg::onRadioButton(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	switch(wID) {
		case IDC_MAGNET_QUEUE:
		case IDC_MAGNET_SEARCH:
			if(mSize > 0 && mFileName.length() > 0) {
				::EnableWindow(GetDlgItem(IDC_MAGNET_REMEMBER), true);
			}
			break;
		case IDC_MAGNET_NOTHING:
			if(IsDlgButtonChecked(IDC_MAGNET_REMEMBER) == BST_CHECKED) {
				::CheckDlgButton(m_hWnd, IDC_MAGNET_REMEMBER, false);
			}
			::EnableWindow(GetDlgItem(IDC_MAGNET_REMEMBER), false);
			break;
	};
	return 0;
}

/**
* @file
* $Id$
*/