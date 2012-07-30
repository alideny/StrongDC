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

#include "stdafx.h"

#include "../client/DCPlusPlus.h"
#include "../client/SettingsManager.h"
#include "../client/FavoriteManager.h"

#include "Resource.h"
#include "WindowsPage.h"
#include "WinUtil.h"

PropPage::Item WindowsPage::items[] = { 
	{ IDC_MAX_TAB_ROWS, SettingsManager::MAX_TAB_ROWS, PropPage::T_INT },
	{ 0, 0, PropPage::T_END } 
};

PropPage::TextItem WindowsPage::textItem[] = {
	{ IDC_SETTINGS_AUTO_OPEN, ResourceManager::SETTINGS_AUTO_OPEN },
	{ IDC_SETTINGS_WINDOWS_OPTIONS, ResourceManager::SETTINGS_WINDOWS_OPTIONS },
	{ IDC_TABSTEXT, ResourceManager::TABS_POSITION },
	{ IDC_SETTINGS_MAX_TAB_ROWS, ResourceManager::SETTINGS_MAX_TAB_ROWS },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

WindowsPage::ListItem WindowsPage::listItems[] = {
	{ SettingsManager::OPEN_PUBLIC, ResourceManager::PUBLIC_HUBS },
	{ SettingsManager::OPEN_FAVORITE_HUBS, ResourceManager::FAVORITE_HUBS },
	{ SettingsManager::OPEN_FAVORITE_USERS, ResourceManager::FAVORITE_USERS },
	{ SettingsManager::OPEN_RECENT_HUBS, ResourceManager::RECENT_HUBS },	
	{ SettingsManager::OPEN_QUEUE, ResourceManager::DOWNLOAD_QUEUE },
	{ SettingsManager::OPEN_FINISHED_DOWNLOADS, ResourceManager::FINISHED_DOWNLOADS },
	{ SettingsManager::OPEN_UPLOAD_QUEUE, ResourceManager::UPLOAD_QUEUE },
	{ SettingsManager::OPEN_FINISHED_UPLOADS, ResourceManager::FINISHED_UPLOADS },
	{ SettingsManager::OPEN_SEARCH_SPY, ResourceManager::SEARCH_SPY },
	{ SettingsManager::OPEN_NETWORK_STATISTICS, ResourceManager::NETWORK_STATISTICS },
	{ SettingsManager::OPEN_NOTEPAD, ResourceManager::NOTEPAD },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

WindowsPage::ListItem WindowsPage::optionItems[] = {
	{ SettingsManager::POPUP_PMS, ResourceManager::SETTINGS_POPUP_PMS },
	{ SettingsManager::POPUP_HUB_PMS, ResourceManager::SETTINGS_POPUP_HUB_PMS },
	{ SettingsManager::POPUP_BOT_PMS, ResourceManager::SETTINGS_POPUP_BOT_PMS },
	{ SettingsManager::POPUNDER_FILELIST, ResourceManager::SETTINGS_POPUNDER_FILELIST },
	{ SettingsManager::POPUNDER_PM, ResourceManager::SETTINGS_POPUNDER_PM },
	{ SettingsManager::JOIN_OPEN_NEW_WINDOW, ResourceManager::SETTINGS_OPEN_NEW_WINDOW },
	{ SettingsManager::IGNORE_HUB_PMS, ResourceManager::SETTINGS_IGNORE_HUB_PMS },
	{ SettingsManager::IGNORE_BOT_PMS, ResourceManager::SETTINGS_IGNORE_BOT_PMS },
	{ SettingsManager::TOGGLE_ACTIVE_WINDOW, ResourceManager::SETTINGS_TOGGLE_ACTIVE_WINDOW },
	{ SettingsManager::PROMPT_PASSWORD, ResourceManager::SETTINGS_PROMPT_PASSWORD },
	{ SettingsManager::CONFIRM_EXIT, ResourceManager::SETTINGS_CONFIRM_EXIT },
	{ SettingsManager::CONFIRM_HUB_REMOVAL, ResourceManager::SETTINGS_CONFIRM_HUB_REMOVAL },
	{ SettingsManager::CONFIRM_DELETE, ResourceManager::SETTINGS_CONFIRM_ITEM_REMOVAL },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT WindowsPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), textItem);
	PropPage::read((HWND)*this, items, listItems, GetDlgItem(IDC_WINDOWS_STARTUP));
	PropPage::read((HWND)*this, items, optionItems, GetDlgItem(IDC_WINDOWS_OPTIONS));

	// Do specialized reading here
	CUpDownCtrl updown;
	updown.Attach(GetDlgItem(IDC_TAB_SPIN));
	updown.SetRange32(1, 10);
	updown.Detach();

	CComboBox tabsPosition;
	tabsPosition.Attach(GetDlgItem(IDC_TABSCOMBO));
	tabsPosition.AddString(CTSTRING(TABS_TOP));
	tabsPosition.AddString(CTSTRING(TABS_BOTTOM));
	tabsPosition.AddString(CTSTRING(TABS_LEFT));
	tabsPosition.AddString(CTSTRING(TABS_RIGHT));
	tabsPosition.SetCurSel(SETTING(TABS_POS));
	tabsPosition.Detach();

	return TRUE;
}

void WindowsPage::write() {
	PropPage::write((HWND)*this, items, listItems, GetDlgItem(IDC_WINDOWS_STARTUP));
	PropPage::write((HWND)*this, items, optionItems, GetDlgItem(IDC_WINDOWS_OPTIONS));

	CComboBox tabsPosition;
	tabsPosition.Attach(GetDlgItem(IDC_TABSCOMBO));
	settings->set(SettingsManager::TABS_POS, tabsPosition.GetCurSel());
	tabsPosition.Detach();
}

/**
 * @file
 * $Id$
 */
