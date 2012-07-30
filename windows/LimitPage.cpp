#include "stdafx.h"

#include "../client/DCPlusPlus.h"
#include "../client/SettingsManager.h"

#include "Resource.h"
#include "LimitPage.h"
#include "WinUtil.h"

PropPage::TextItem LimitPage::texts[] = {
	{ IDC_THROTTLE_ENABLE, ResourceManager::SETSTRONGDC_ENABLE_LIMITING },
	{ IDC_STRONGDC_TRANSFER_LIMITING, ResourceManager::SETSTRONGDC_TRANSFER_LIMITING },
	{ IDC_STRONGDC_UP_SPEED, ResourceManager::SETSTRONGDC_UPLOAD_SPEED },
	{ IDC_STRONGDC_UP_SPEED1, ResourceManager::SETSTRONGDC_UPLOAD_SPEED },
	{ IDC_SETTINGS_KBPS1, ResourceManager::KBPS },
	{ IDC_SETTINGS_KBPS2, ResourceManager::KBPS },
	{ IDC_SETTINGS_KBPS3, ResourceManager::KBPS },
	{ IDC_SETTINGS_KBPS4, ResourceManager::KBPS },
	{ IDC_SETTINGS_KBPS5, ResourceManager::KBPS },
	{ IDC_SETTINGS_KBPS6, ResourceManager::KBPS },
	{ IDC_SETTINGS_KBPS7, ResourceManager::KBPS },
	{ IDC_SETTINGS_MINUTES, ResourceManager::SECONDS },
	{ IDC_STRONGDC_DW_SPEED, ResourceManager::SETSTRONGDC_DOWNLOAD_SPEED },
	{ IDC_STRONGDC_DW_SPEED1, ResourceManager::SETSTRONGDC_DOWNLOAD_SPEED },
	{ IDC_TIME_LIMITING, ResourceManager::SETSTRONGDC_ALTERNATE_LIMITING },
	{ IDC_STRONGDC_TO, ResourceManager::SETSTRONGDC_TO },
	{ IDC_STRONGDC_SECONDARY_TRANSFER, ResourceManager::SETSTRONGDC_SECONDARY_LIMITING },
	{ IDC_STRONGDC_UP_NOTE, ResourceManager::SETSTRONGDC_NOTE_UPLOAD },
	{ IDC_STRONGDC_DW_NOTE, ResourceManager::SETSTRONGDC_NOTE_DOWNLOAD },
	{ IDC_STRONGDC_SLOW_DISCONNECT, ResourceManager::SETSTRONGDC_SLOW_DISCONNECT },
	{ IDC_SEGMENTED_ONLY, ResourceManager::SETTINGS_AUTO_DROP_SEGMENTED_SOURCE },
	{ IDC_STRONGDC_I_DOWN_SPEED, ResourceManager::SETSTRONGDC_I_DOWN_SPEED },
	{ IDC_STRONGDC_TIME_DOWN, ResourceManager::SETSTRONGDC_TIME_DOWN },
	{ IDC_STRONGDC_H_DOWN_SPEED, ResourceManager::SETSTRONGDC_H_DOWN_SPEED },
	{ IDC_STRONGDC_MIN_FILE_SIZE, ResourceManager::SETSTRONGDC_MIN_FILE_SIZE },
	{ IDC_SETTINGS_MB, ResourceManager::MB },
	{ IDC_REMOVE_IF, ResourceManager::NEW_DISCONNECT },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
}; 

PropPage::Item LimitPage::items[] = {
	{ IDC_MX_UP_SP_LMT_NORMAL, SettingsManager::MAX_UPLOAD_SPEED_LIMIT, PropPage::T_INT },
	{ IDC_MX_DW_SP_LMT_NORMAL, SettingsManager::MAX_DOWNLOAD_SPEED_LIMIT, PropPage::T_INT },
	{ IDC_TIME_LIMITING, SettingsManager::TIME_DEPENDENT_THROTTLE, PropPage::T_BOOL },
	{ IDC_MX_UP_SP_LMT_TIME, SettingsManager::MAX_UPLOAD_SPEED_LIMIT_TIME, PropPage::T_INT },
	{ IDC_MX_DW_SP_LMT_TIME, SettingsManager::MAX_DOWNLOAD_SPEED_LIMIT_TIME, PropPage::T_INT },
	{ IDC_BW_START_TIME, SettingsManager::BANDWIDTH_LIMIT_START, PropPage::T_INT },
	{ IDC_BW_END_TIME, SettingsManager::BANDWIDTH_LIMIT_END, PropPage::T_INT },
	{ IDC_THROTTLE_ENABLE, SettingsManager::THROTTLE_ENABLE, PropPage::T_BOOL },
	{ IDC_I_DOWN_SPEED, SettingsManager::DISCONNECT_SPEED, PropPage::T_INT },
	{ IDC_TIME_DOWN, SettingsManager::DISCONNECT_TIME, PropPage::T_INT },
	{ IDC_H_DOWN_SPEED, SettingsManager::DISCONNECT_FILE_SPEED, PropPage::T_INT },
	{ IDC_SEGMENTED_ONLY, SettingsManager::DROP_MULTISOURCE_ONLY, PropPage::T_BOOL },
	{ IDC_MIN_FILE_SIZE, SettingsManager::DISCONNECT_FILESIZE, PropPage::T_INT },
	{ IDC_REMOVE_IF_BELOW, SettingsManager::REMOVE_SPEED, PropPage::T_INT },
	{ 0, 0, PropPage::T_END }
};

LRESULT LimitPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items);

	CUpDownCtrl spin;
	spin.Attach(GetDlgItem(IDC_I_DOWN_SPEED_SPIN));
	spin.SetRange32(1, 99999);
	spin.Detach(); 
	spin.Attach(GetDlgItem(IDC_TIME_DOWN_SPIN));
	spin.SetRange32(10, 180);
	spin.Detach(); 
	spin.Attach(GetDlgItem(IDC_H_DOWN_SPEED_SPIN));
	spin.SetRange32(0, 4096);
	spin.Detach(); 
	spin.Attach(GetDlgItem(IDC_UPLOADSPEEDSPIN));
	spin.SetRange32(0, 99999);
	spin.Detach(); 
	spin.Attach(GetDlgItem(IDC_DOWNLOADSPEEDSPIN));
	spin.SetRange32(0, 99999);
	spin.Detach(); 
	spin.Attach(GetDlgItem(IDC_UPLOADSPEEDSPIN_TIME));
	spin.SetRange32(0, 99999);
	spin.Detach(); 
	spin.Attach(GetDlgItem(IDC_DOWNLOADSPEEDSPIN_TIME));
	spin.SetRange32(0, 99999);
	spin.Detach(); 
	spin.Attach(GetDlgItem(IDC_MIN_FILE_SIZE_SPIN));
	spin.SetRange32(0, 4096);
	spin.Detach(); 
	spin.Attach(GetDlgItem(IDC_REMOVE_SPIN));
	spin.SetRange32(0, 99999);
	spin.Detach(); 

	timeCtrlBegin.Attach(GetDlgItem(IDC_BW_START_TIME));
	timeCtrlEnd.Attach(GetDlgItem(IDC_BW_END_TIME));

	timeCtrlBegin.AddString(_T("Midnight"));
	timeCtrlEnd.AddString(_T("Midnight"));
	for (int i = 1; i < 12; ++i)
	{
		timeCtrlBegin.AddString((Util::toStringW(i) + _T(" AM")).c_str());
		timeCtrlEnd.AddString((Util::toStringW(i) + _T(" AM")).c_str());
	}
	timeCtrlBegin.AddString(_T("Noon"));
	timeCtrlEnd.AddString(_T("Noon"));
	for (int i = 1; i < 12; ++i)
	{
		timeCtrlBegin.AddString((Util::toStringW(i) + _T(" PM")).c_str());
		timeCtrlEnd.AddString((Util::toStringW(i) + _T(" PM")).c_str());
	}

	timeCtrlBegin.SetCurSel(SETTING(BANDWIDTH_LIMIT_START));
	timeCtrlEnd.SetCurSel(SETTING(BANDWIDTH_LIMIT_END));

	timeCtrlBegin.Detach();
	timeCtrlEnd.Detach();

	fixControls();

	// Do specialized reading here

	return TRUE;
}

void LimitPage::write()
{
	PropPage::write((HWND)*this, items);

	// Do specialized writing here
	// settings->set(XX, YY);

	timeCtrlBegin.Attach(GetDlgItem(IDC_BW_START_TIME));
	timeCtrlEnd.Attach(GetDlgItem(IDC_BW_END_TIME));
	settings->set(SettingsManager::BANDWIDTH_LIMIT_START, timeCtrlBegin.GetCurSel());
	settings->set(SettingsManager::BANDWIDTH_LIMIT_END, timeCtrlEnd.GetCurSel());
	timeCtrlBegin.Detach();
	timeCtrlEnd.Detach(); 
}

void LimitPage::fixControls() {
	bool state = (IsDlgButtonChecked(IDC_THROTTLE_ENABLE) != 0);
	::EnableWindow(GetDlgItem(IDC_MX_UP_SP_LMT_NORMAL), state);
	::EnableWindow(GetDlgItem(IDC_UPLOADSPEEDSPIN), state);
	::EnableWindow(GetDlgItem(IDC_MX_DW_SP_LMT_NORMAL), state);
	::EnableWindow(GetDlgItem(IDC_DOWNLOADSPEEDSPIN), state);
	::EnableWindow(GetDlgItem(IDC_TIME_LIMITING), state);

	state = ((IsDlgButtonChecked(IDC_THROTTLE_ENABLE) != 0) && (IsDlgButtonChecked(IDC_TIME_LIMITING) != 0));
	::EnableWindow(GetDlgItem(IDC_BW_START_TIME), state);
	::EnableWindow(GetDlgItem(IDC_BW_END_TIME), state);
	::EnableWindow(GetDlgItem(IDC_MX_UP_SP_LMT_TIME), state);
	::EnableWindow(GetDlgItem(IDC_UPLOADSPEEDSPIN_TIME), state);
	::EnableWindow(GetDlgItem(IDC_MX_DW_SP_LMT_TIME), state);
	::EnableWindow(GetDlgItem(IDC_DOWNLOADSPEEDSPIN_TIME), state);
}

LRESULT LimitPage::onChangeCont(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	switch (wID) {
	case IDC_TIME_LIMITING:
	case IDC_THROTTLE_ENABLE:
		fixControls();
		break;
	}
	return true;
}