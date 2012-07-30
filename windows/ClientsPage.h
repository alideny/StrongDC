#ifndef CLIENTSPAGE_H
#define CLIENTSPAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "PropPage.h"
#include "ExListViewCtrl.h"

#include "../client/HttpConnection.h"
#include "../client/File.h"
#include "../client/DetectionManager.h"

class ClientsPage : public CPropertyPage<IDD_DETECTION_LIST_PAGE>, public PropPage, private HttpConnectionListener {
public:
	enum { WM_PROFILE = WM_APP + 53 };

	ClientsPage(SettingsManager *s) : PropPage(s) {
		title = _tcsdup((TSTRING(SETTINGS_ADVANCED) + _T('\\') + TSTRING(SETTINGS_FAKEDETECT) + _T('\\') + TSTRING(SETTINGS_CLIENTS)).c_str());
		SetTitle(title);
		m_psp.dwFlags |= PSP_RTLREADING;
		c.addListener(this);
	};

	~ClientsPage() { 
		ctrlProfiles.Detach();
		free(title);
		c.removeListener(this);
	};

	BEGIN_MSG_MAP(ClientsPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_ID_HANDLER(IDC_ADD_CLIENT, onAddClient)
		COMMAND_ID_HANDLER(IDC_REMOVE_CLIENT, onRemoveClient)
		COMMAND_ID_HANDLER(IDC_CHANGE_CLIENT, onChangeClient)
		COMMAND_ID_HANDLER(IDC_MOVE_CLIENT_UP, onMoveClientUp)
		COMMAND_ID_HANDLER(IDC_MOVE_CLIENT_DOWN, onMoveClientDown)
		COMMAND_ID_HANDLER(IDC_RELOAD_CLIENTS, onReload)
		COMMAND_ID_HANDLER(IDC_UPDATE, onUpdate)
		NOTIFY_HANDLER(IDC_CLIENT_ITEMS, NM_CUSTOMDRAW, onCustomDraw)
		NOTIFY_HANDLER(IDC_CLIENT_ITEMS, NM_DBLCLK, onDblClick)
		NOTIFY_HANDLER(IDC_CLIENT_ITEMS, LVN_GETINFOTIP, onInfoTip)
		NOTIFY_HANDLER(IDC_CLIENT_ITEMS, LVN_ITEMCHANGED, onItemchangedDirectories)
		NOTIFY_HANDLER(IDC_CLIENT_ITEMS, LVN_KEYDOWN, onKeyDown)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT, WPARAM, LPARAM, BOOL&);

	LRESULT onAddClient(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onChangeClient(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRemoveClient(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onMoveClientUp(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onMoveClientDown(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onReload(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onUpdate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onInfoTip(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onItemchangedDirectories(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);

	LRESULT onDblClick(int /*idCtrl*/, LPNMHDR /* pnmh */, BOOL& bHandled) {
		return onChangeClient(0, 0, 0, bHandled);
	}
	LRESULT onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	// Common PropPage interface
	PROPSHEETPAGE *getPSP() { return (PROPSHEETPAGE *)*this; }
	void write();
	
private:
	ExListViewCtrl ctrlProfiles;

	static Item items[];
	static TextItem texts[];
	TCHAR* title;

	void addEntry(const DetectionEntry& de, int pos);
	void reload();
	void reloadFromHttp();

	void on(HttpConnectionListener::Data, HttpConnection* /*conn*/, const uint8_t* buf, size_t len) throw() {
		downBuf.append((char*)buf, len);
	}
	void on(HttpConnectionListener::Complete, HttpConnection* conn, string const& /*aLine*/, bool /*fromCoral*/) throw() {
		conn->removeListener(this);
		if(!downBuf.empty()) {
			string fname = Util::getPath(Util::PATH_USER_CONFIG) + "Profiles.xml";
			File f(fname + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
			f.write(downBuf);
			f.close();
			File::deleteFile(fname);
			File::renameFile(fname + ".tmp", fname);
			reloadFromHttp();
			MessageBox(_T("Client profiles now updated."), _T("Updated"), MB_OK);
		}
		::EnableWindow(GetDlgItem(IDC_UPDATE), true);
	}
	void on(HttpConnectionListener::Failed, HttpConnection* conn, const string& aLine) throw() {
		conn->removeListener(this);
		{
			string msg = "Client profiles download failed.\r\n" + aLine;
			MessageBox(Text::toT(msg).c_str(), _T("Failed"), MB_OK);
		}
		::EnableWindow(GetDlgItem(IDC_UPDATE), true);
	}
	HttpConnection c;
	string downBuf;
};

#endif //CLIENTSPAGE_H

/**
 * @file
 * $Id$
 */
