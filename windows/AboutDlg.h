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

#if !defined(ABOUT_DLG_H)
#define ABOUT_DLG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../client/HttpConnection.h"
#include "../client/SimpleXML.h"

static const TCHAR thanks[] = 
_T("D�kuji v�em, kte�� m� ve v�voji podporovali. THX: Andyman (for muscle-arm logo), Atom, Blackrabbit, Catalin, ")
_T("Chmelic, Cinique, Corvik, Crakter, darwusch, Dreamveawer, FarCry, Fartman, GargoyleMT, Gigadisk (for Czech translation), ")
_T("Ice, Jove, King Wenceslas, klirik, Lee, Legolas, Liny, MaynardK, Mlok, Naga, Patrol, popKorn, Pothead, ProLogic, Rm., ")
_T("Testament, Warrior, WereWiking, xAyiDe, XiND and ToH+trup for very nice betatesting :-) and to all donators who support me: ")
_T("Kulmegil, Michal Seckar, k3dt, majki, mazeboy, Fino, Architect, Ujep, Jeepo, mlok, redsaq, anton009, Lee, share2all, ")
_T("Dr. Martin Sereday, Jozef Akantis, Halfspider, Khalid Talbi, Shine Developer");

class AboutDlg : public CDialogImpl<AboutDlg>, private HttpConnectionListener
{
public:
	enum { IDD = IDD_ABOUTBOX };
	enum { WM_VERSIONDATA = WM_APP + 53 };

	AboutDlg() { }
	~AboutDlg() { }

	BEGIN_MSG_MAP(AboutDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_VERSIONDATA, onVersionData)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		SetDlgItemText(IDC_VERSION, _T("StrongDC++ v") _T(VERSIONSTRING) _T(" (c) Copyright 2004-2010 Big Muscle\nBased on: DC++ ") _T(DCVERSIONSTRING) _T(" (c) Copyright 2001-2010 Jacek Sieka\n\nhttp://strongdc.sf.net"));
		CEdit ctrlThanks(GetDlgItem(IDC_THANKS));
		ctrlThanks.FmtLines(TRUE);
		ctrlThanks.AppendText(thanks, TRUE);
		ctrlThanks.Detach();
		SetDlgItemText(IDC_TTH, WinUtil::tth.c_str());
		SetDlgItemText(IDC_LATEST, CTSTRING(DOWNLOADING));
		SetDlgItemText(IDC_TOTALS, (_T("Upload: ") + Util::formatBytesW(SETTING(TOTAL_UPLOAD)) + _T(", Download: ") + 
			Util::formatBytesW(SETTING(TOTAL_DOWNLOAD))).c_str());

		if(SETTING(TOTAL_DOWNLOAD) > 0) {
			TCHAR buf[64];
			snwprintf(buf, sizeof(buf), _T("Ratio (up/down): %.2f"), ((double)SETTING(TOTAL_UPLOAD)) / ((double)SETTING(TOTAL_DOWNLOAD)));

			SetDlgItemText(IDC_RATIO, buf);
		/*	sprintf(buf, "Uptime: %s", Util::formatTime(Util::getUptime()));
			SetDlgItemText(IDC_UPTIME, Text::toT(buf).c_str());*/
		}
		CenterWindow(GetParent());
		c.addListener(this);
		c.downloadFile(VERSION_URL);
		return TRUE;
	}

	LRESULT onVersionData(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		tstring* x = (tstring*) wParam;
		SetDlgItemText(IDC_LATEST, x->c_str());
		delete x;
		return 0;
	}
		
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		EndDialog(wID);
		return 0;
	}

private:
	HttpConnection c;

	AboutDlg(const AboutDlg&) { dcassert(0); }
	
	void on(HttpConnectionListener::Data, HttpConnection* /*conn*/, const uint8_t* buf, size_t len) throw() {
		downBuf.append((char*)buf, len);
	}

	void on(HttpConnectionListener::Complete, HttpConnection* conn, const string&, bool /*fromCoral*/) throw() {
		if(!downBuf.empty()) {
			try {
				SimpleXML xml;
				xml.fromXML(downBuf);
				if(xml.findChild("DCUpdate")) {
					xml.stepIn();
					if(xml.findChild("Version")) {
						tstring* x = new tstring(Text::toT(xml.getChildData()));
						PostMessage(WM_VERSIONDATA, (WPARAM) x);
					}
				}
			} catch(const SimpleXMLException&) { }
		}
		conn->removeListener(this);
	}

	void on(HttpConnectionListener::Failed, HttpConnection* conn, const string& aLine) throw() {
		tstring* x = new tstring(Text::toT(aLine));
		PostMessage(WM_VERSIONDATA, (WPARAM) x);
		conn->removeListener(this);
	}

	string downBuf;
};

#endif // !defined(ABOUT_DLG_H)

/**
 * @file
 * $Id$
 */
