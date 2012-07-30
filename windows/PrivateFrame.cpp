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
#include "Resource.h"

#include "PrivateFrame.h"
#include "SearchFrm.h"
#include "WinUtil.h"
#include "MainFrm.h"
#include "EmoticonsManager.h"

#include "../client/Client.h"
#include "../client/ClientManager.h"
#include "../client/Util.h"
#include "../client/LogManager.h"
#include "../client/UploadManager.h"
#include "../client/ShareManager.h"
#include "../client/FavoriteManager.h"
#include "../client/QueueManager.h"
#include "../client/StringTokenizer.h"

PrivateFrame::FrameMap PrivateFrame::frames;
tstring pSelectedLine = Util::emptyStringT;
tstring pSelectedURL = Util::emptyStringT;

extern EmoticonsManager* emoticonsManager;

LRESULT PrivateFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);
	
	ctrlClient.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL | ES_READONLY, WS_EX_CLIENTEDGE, IDC_CLIENT);
	
	ctrlClientContainer.SubclassWindow(ctrlClient.m_hWnd);
	ctrlClient.Subclass();
	ctrlClient.LimitText(0);
	ctrlClient.SetFont(WinUtil::font);
	
	ctrlClient.SetBackgroundColor( SETTING(BACKGROUND_COLOR) ); 
	ctrlClient.SetAutoURLDetect(false);
	ctrlClient.SetEventMask( ctrlClient.GetEventMask() | ENM_LINK );
	ctrlMessage.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		ES_AUTOHSCROLL | ES_MULTILINE | ES_AUTOVSCROLL, WS_EX_CLIENTEDGE);
	
	ctrlMessageContainer.SubclassWindow(ctrlMessage.m_hWnd);

	ctrlMessage.SetFont(WinUtil::font);
	ctrlMessage.SetLimitText(9999);

	ctrlEmoticons.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_FLAT | BS_BITMAP | BS_CENTER, 0, IDC_EMOT);

	hEmoticonBmp = (HBITMAP) ::LoadImage(_Module.get_m_hInst(), MAKEINTRESOURCE(IDB_EMOTICON), IMAGE_BITMAP, 0, 0, LR_SHARED);
  	ctrlEmoticons.SetBitmap(hEmoticonBmp);

	PostMessage(WM_SPEAKER, USER_UPDATED);
	created = true;

	ClientManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);

	readLog();

	bHandled = FALSE;
	return 1;
}

void PrivateFrame::gotMessage(const Identity& from, const UserPtr& to, const UserPtr& replyTo, const tstring& aMessage, Client* c) {
	PrivateFrame* p = NULL;
	bool myPM = replyTo == ClientManager::getInstance()->getMe();
	const UserPtr& user = myPM ? to : replyTo;
	
	FrameIter i = frames.find(user);
	if(i == frames.end()) {
		if(frames.size() > 200) return;
		p = new PrivateFrame(HintedUser(user, c->getHubUrl()), c);
		frames[user] = p;
		
		p->addLine(from, aMessage);

		if(Util::getAway()) {
			if(!(BOOLSETTING(NO_AWAYMSG_TO_BOTS) && user->isSet(User::BOT))) 
			{
				StringMap params;
				from.getParams(params, "user", false);
				p->sendMessage(Text::toT(Util::getAwayMessage(params)));
			}
		}

		if(BOOLSETTING(POPUP_NEW_PM)) {
			pair<tstring, bool> hubs = WinUtil::getHubNames(replyTo, c->getHubUrl());
			MainFrame::getMainFrame()->ShowBalloonTip(WinUtil::getNicks(replyTo, c->getHubUrl()) + _T(" - ") + hubs.first, TSTRING(PRIVATE_MESSAGE));
		}

		if((BOOLSETTING(PRIVATE_MESSAGE_BEEP) || BOOLSETTING(PRIVATE_MESSAGE_BEEP_OPEN)) && (!BOOLSETTING(SOUNDS_DISABLED))) {
			if (SETTING(BEEPFILE).empty()) {
				MessageBeep(MB_OK);
			} else {
				::PlaySound(Text::toT(SETTING(BEEPFILE)).c_str(), NULL, SND_FILENAME | SND_ASYNC);
			}
		}
	} else {
		if(!myPM) {
			if(BOOLSETTING(POPUP_PM)) {
				pair<tstring, bool> hubs = WinUtil::getHubNames(replyTo, c->getHubUrl());
				MainFrame::getMainFrame()->ShowBalloonTip(WinUtil::getNicks(replyTo, c->getHubUrl()) + _T(" - ") + hubs.first, TSTRING(PRIVATE_MESSAGE));
			}

			if((BOOLSETTING(PRIVATE_MESSAGE_BEEP)) && (!BOOLSETTING(SOUNDS_DISABLED))) {
				if (SETTING(BEEPFILE).empty()) {
					MessageBeep(MB_OK);
				} else {
					::PlaySound(Text::toT(SETTING(BEEPFILE)).c_str(), NULL, SND_FILENAME | SND_ASYNC);
				}
			}
		}
		i->second->addLine(from, aMessage);
	}
}

void PrivateFrame::openWindow(const HintedUser& replyTo, const tstring& msg, Client* c) {
	PrivateFrame* p = NULL;
	FrameIter i = frames.find(replyTo);
	if(i == frames.end()) {
		if(frames.size() > 200) return;
		p = new PrivateFrame(replyTo, c);
		frames[replyTo] = p;
		p->CreateEx(WinUtil::mdiClient);
	} else {
		p = i->second;
		if(::IsIconic(p->m_hWnd))
			::ShowWindow(p->m_hWnd, SW_RESTORE);
		p->MDIActivate(p->m_hWnd);
	}
	if(!msg.empty())
		p->sendMessage(msg);
}

LRESULT PrivateFrame::onChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	if (uMsg != WM_KEYDOWN) {
		switch(wParam) {
			case VK_RETURN:
				if( WinUtil::isShift() || WinUtil::isCtrl() ||  WinUtil::isAlt() ) {
					bHandled = FALSE;
				}
				break;
		case VK_TAB:
				bHandled = TRUE;
  				break;
  			default:
  				bHandled = FALSE;
				break;
		}
		if ((uMsg == WM_CHAR) && (GetFocus() == ctrlMessage.m_hWnd) && (wParam != VK_RETURN) && (wParam != VK_TAB) && (wParam != VK_BACK)) {
			if ((!SETTING(SOUND_TYPING_NOTIFY).empty()) && (!BOOLSETTING(SOUNDS_DISABLED)))
				PlaySound(Text::toT(SETTING(SOUND_TYPING_NOTIFY)).c_str(), NULL, SND_FILENAME | SND_ASYNC);
		}
		return 0;
	}
	
	switch(wParam) {
		case VK_TAB:
		{
			if(GetFocus() == ctrlMessage.m_hWnd)
			{
				ctrlClient.SetFocus();
			}
			else
			{
				ctrlMessage.SetFocus();
			}
		}	
	}
	
	// don't handle these keys unless the user is entering a message
	if (GetFocus() != ctrlMessage.m_hWnd) {
		bHandled = FALSE;
		return 0;
	}	
	
	switch(wParam) {
		case VK_RETURN:
			if( (GetKeyState(VK_SHIFT) & 0x8000) || 
				(GetKeyState(VK_CONTROL) & 0x8000) || 
				(GetKeyState(VK_MENU) & 0x8000) ) {
				bHandled = FALSE;
			} else {
				if(uMsg == WM_KEYDOWN) {
					onEnter();
				}
			}
			break;
		case VK_UP:
			if ((GetKeyState(VK_CONTROL) & 0x8000) || (GetKeyState(VK_MENU) & 0x8000)) {
				//scroll up in chat command history
				//currently beyond the last command?
				if (curCommandPosition > 0) {
					//check whether current command needs to be saved
					if (curCommandPosition == prevCommands.size()) {
						currentCommand.resize(ctrlMessage.GetWindowTextLength());
						ctrlMessage.GetWindowText(&currentCommand[0], ctrlMessage.GetWindowTextLength() + 1);
					}
					//replace current chat buffer with current command
					ctrlMessage.SetWindowText(prevCommands[--curCommandPosition].c_str());
				}
			} else {
				bHandled = FALSE;
			}
			break;
		case VK_DOWN:
			if ((GetKeyState(VK_CONTROL) & 0x8000) || (GetKeyState(VK_MENU) & 0x8000)) {
				//scroll down in chat command history
				//currently beyond the last command?
				if (curCommandPosition + 1 < prevCommands.size()) {
					//replace current chat buffer with current command
					ctrlMessage.SetWindowText(prevCommands[++curCommandPosition].c_str());
				} else if (curCommandPosition + 1 == prevCommands.size()) {
					//revert to last saved, unfinished command
					ctrlMessage.SetWindowText(currentCommand.c_str());
					++curCommandPosition;
				}
			} else {
				bHandled = FALSE;
			}
			break;
		case VK_HOME:
			if (!prevCommands.empty() && (GetKeyState(VK_CONTROL) & 0x8000) || (GetKeyState(VK_MENU) & 0x8000)) {
				curCommandPosition = 0;
				currentCommand.resize(ctrlMessage.GetWindowTextLength());
				ctrlMessage.GetWindowText(&currentCommand[0], ctrlMessage.GetWindowTextLength() + 1);
				ctrlMessage.SetWindowText(prevCommands[curCommandPosition].c_str());
			} else {
				bHandled = FALSE;
			}
			break;
		case VK_END:
			if ((GetKeyState(VK_CONTROL) & 0x8000) || (GetKeyState(VK_MENU) & 0x8000)) {
				curCommandPosition = prevCommands.size();
				ctrlMessage.SetWindowText(currentCommand.c_str());
			} else {
				bHandled = FALSE;
			}
			break;
		default:
			bHandled = FALSE;
	}
	return 0;
}

void PrivateFrame::onEnter()
{
	bool resetText = true;

	if(ctrlMessage.GetWindowTextLength() > 0) {
		tstring s;
		s.resize(ctrlMessage.GetWindowTextLength());
		
		ctrlMessage.GetWindowText(&s[0], s.size() + 1);

		// save command in history, reset current buffer pointer to the newest command
		curCommandPosition = prevCommands.size();		//this places it one position beyond a legal subscript
		if (!curCommandPosition || curCommandPosition > 0 && prevCommands[curCommandPosition - 1] != s) {
			++curCommandPosition;
			prevCommands.push_back(s);
		}
		currentCommand = Util::emptyStringT;

		// Process special commands
		if(s[0] == '/') {
			tstring m = s;
			tstring param;
			tstring message;
			tstring status;
			bool thirdPerson = false;
			if(WinUtil::checkCommand(s, param, message, status, thirdPerson)) {
				if(!message.empty()) {
					sendMessage(message, thirdPerson);
				}
				if(!status.empty()) {
					addClientLine(status);
				}
			} else if((stricmp(s.c_str(), _T("clear")) == 0) || (stricmp(s.c_str(), _T("cls")) == 0)) {
				ctrlClient.SetWindowText(_T(""));
			} else if(stricmp(s.c_str(), _T("grant")) == 0) {
				UploadManager::getInstance()->reserveSlot(HintedUser(replyTo, replyTo.hint), 600);
				addClientLine(TSTRING(SLOT_GRANTED));
			} else if(stricmp(s.c_str(), _T("close")) == 0) {
				PostMessage(WM_CLOSE);
			} else if((stricmp(s.c_str(), _T("favorite")) == 0) || (stricmp(s.c_str(), _T("fav")) == 0)) {
				FavoriteManager::getInstance()->addFavoriteUser(replyTo);
				addClientLine(TSTRING(FAVORITE_USER_ADDED));
			} else if(stricmp(s.c_str(), _T("getlist")) == 0) {
				BOOL bTmp;
				onGetList(0,0,0,bTmp);
			} else if(stricmp(s.c_str(), _T("log")) == 0) {
				StringMap params;
				const CID& cid = replyTo.user->getCID();
				const string& hint = replyTo.hint;
	
				params["hubNI"] = Util::toString(ClientManager::getInstance()->getHubNames(cid, hint, priv));
				params["hubURL"] = Util::toString(ClientManager::getInstance()->getHubs(cid, hint, priv));
				params["userCID"] = cid.toBase32(); 
				params["userNI"] = ClientManager::getInstance()->getNicks(cid, hint, priv)[0];
				params["myCID"] = ClientManager::getInstance()->getMe()->getCID().toBase32();
				WinUtil::openFile(Text::toT(Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatParams(SETTING(LOG_FILE_PRIVATE_CHAT), params, false))));
			} else if(stricmp(s.c_str(), _T("stats")) == 0) {
				sendMessage(Text::toT(WinUtil::generateStats()));
			} else if(stricmp(s.c_str(), _T("help")) == 0) {
				addLine(_T("*** ") + WinUtil::commands + _T(", /getlist, /clear, /grant, /close, /favorite, /winamp"), WinUtil::m_ChatTextSystem);
			} else {
				if(replyTo.user->isOnline()) {
					if (BOOLSETTING(SEND_UNKNOWN_COMMANDS)) {
						sendMessage(tstring(m));
					} else {
						addClientLine(TSTRING(UNKNOWN_COMMAND) + _T(" ") + m);
					}
				} else {
					ctrlStatus.SetText(0, CTSTRING(USER_WENT_OFFLINE));
					resetText = false;
				}
			}
		} else {
			if(replyTo.user->isOnline()) {
				if(BOOLSETTING(CZCHARS_DISABLE))
					s = WinUtil::disableCzChars(s);

				sendMessage(s);
			} else {
				ctrlStatus.SetText(0, CTSTRING(USER_WENT_OFFLINE));
				resetText = false;
			}
		}
		if(resetText)
			ctrlMessage.SetWindowText(_T(""));
	} 
}

void PrivateFrame::sendMessage(const tstring& msg, bool thirdPerson) {
	ClientManager::getInstance()->privateMessage(replyTo, Text::fromT(msg), thirdPerson);
}

LRESULT PrivateFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
		DeleteObject(hEmoticonBmp);
		ClientManager::getInstance()->removeListener(this);
		SettingsManager::getInstance()->removeListener(this);
		closed = true;
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		frames.erase(replyTo);

		bHandled = FALSE;
		return 0;
	}
}

void PrivateFrame::addLine(const tstring& aLine, CHARFORMAT2& cf) {
	Identity i = Identity(NULL, 0);
    addLine(i, aLine, cf);
}

void PrivateFrame::addLine(const Identity& from, const tstring& aLine) {
	addLine(from, aLine, WinUtil::m_ChatTextGeneral );
}

void PrivateFrame::addLine(const Identity& from, const tstring& aLine, CHARFORMAT2& cf) {
	if(!created) {
		if(BOOLSETTING(POPUNDER_PM))
			WinUtil::hiddenCreateEx(this);
		else
			CreateEx(WinUtil::mdiClient);
	}

	CRect r;
	ctrlClient.GetClientRect(r);

	if(BOOLSETTING(LOG_PRIVATE_CHAT)) {
		StringMap params;
		const CID& cid = replyTo.user->getCID();
		const string& hint = replyTo.hint;
		
		params["message"] = Text::fromT(aLine);
		params["hubNI"] = Util::toString(ClientManager::getInstance()->getHubNames(cid, hint, priv));
		params["hubURL"] = Util::toString(ClientManager::getInstance()->getHubs(cid, hint, priv));
		params["userCID"] = cid.toBase32(); 
		params["userNI"] = ClientManager::getInstance()->getNicks(cid, hint, priv)[0];
		params["myCID"] = ClientManager::getInstance()->getMe()->getCID().toBase32();
		LOG(LogManager::PM, params);
	}

	if(BOOLSETTING(TIME_STAMPS)) {
		ctrlClient.AppendText(from, Text::toT(SETTING(NICK)), Text::toT("[" + Util::getShortTimeString() + "] "), aLine + _T('\n'), cf);
	} else {
		ctrlClient.AppendText(from, Text::toT(SETTING(NICK)), _T(""), aLine + _T('\n'), cf);
	}
	addClientLine(TSTRING(LAST_CHANGE) + _T(" ") + Text::toT(Util::getTimeString()));

	if (BOOLSETTING(BOLD_PM)) {
		setDirty();
	}
}

LRESULT PrivateFrame::onEditClearAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ctrlClient.SetWindowText(_T(""));
	return 0;
}

LRESULT PrivateFrame::onTabContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click 

	OMenu tabMenu;
	tabMenu.CreatePopupMenu();	

	tabMenu.InsertSeparatorFirst(Text::toT(ClientManager::getInstance()->getNicks(replyTo.user->getCID(), replyTo.hint, priv)[0]));
	if(BOOLSETTING(LOG_PRIVATE_CHAT)) {
		tabMenu.AppendMenu(MF_STRING, IDC_OPEN_USER_LOG,  CTSTRING(OPEN_USER_LOG));
		tabMenu.AppendMenu(MF_SEPARATOR);
	}
	tabMenu.AppendMenu(MF_STRING, ID_EDIT_CLEAR_ALL, CTSTRING(CLEAR));
	tabMenu.AppendMenu(MF_SEPARATOR);
	tabMenu.AppendMenu(MF_STRING, IDC_GETLIST, CTSTRING(GET_FILE_LIST));
	tabMenu.AppendMenu(MF_STRING, IDC_MATCH_QUEUE, CTSTRING(MATCH_QUEUE));
	tabMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)WinUtil::grantMenu, CTSTRING(GRANT_SLOTS_MENU));
	tabMenu.AppendMenu(MF_STRING, IDC_ADD_TO_FAVORITES, CTSTRING(ADD_TO_FAVORITES));

	prepareMenu(tabMenu, UserCommand::CONTEXT_CHAT, ClientManager::getInstance()->getHubs(replyTo.user->getCID(), replyTo.hint, priv));
	if(!(tabMenu.GetMenuState(tabMenu.GetMenuItemCount()-1, MF_BYPOSITION) & MF_SEPARATOR)) {	
		tabMenu.AppendMenu(MF_SEPARATOR);
	}
	tabMenu.AppendMenu(MF_STRING, IDC_CLOSE_WINDOW, CTSTRING(CLOSE));

	tabMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
	return TRUE;
}

void PrivateFrame::runUserCommand(UserCommand& uc) {

	if(!WinUtil::getUCParams(m_hWnd, uc, ucLineParams))
		return;

	StringMap ucParams = ucLineParams;

	ClientManager::getInstance()->userCommand(replyTo, uc, ucParams, true);
}

LRESULT PrivateFrame::onReport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ClientManager::getInstance()->reportUser(HintedUser(replyTo, replyTo.hint));
	return 0;
}

LRESULT PrivateFrame::onCheckList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	try {
		QueueManager::getInstance()->addList(HintedUser(replyTo, replyTo.hint), QueueItem::FLAG_USER_CHECK);
	} catch(const Exception& e) {
		LogManager::getInstance()->message(e.getError());		
	}
	
	return 0;
}

LRESULT PrivateFrame::onGetList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	try {
		QueueManager::getInstance()->addList(HintedUser(replyTo, replyTo.hint), QueueItem::FLAG_CLIENT_VIEW);
	} catch(const Exception& e) {
		addClientLine(Text::toT(e.getError()));
	}
	return 0;
}

LRESULT PrivateFrame::onMatchQueue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	try {
		QueueManager::getInstance()->addList(HintedUser(replyTo, replyTo.hint), QueueItem::FLAG_MATCH_QUEUE);
	} catch(const Exception& e) {
		addClientLine(Text::toT(e.getError()));
	}
	return 0;
}

LRESULT PrivateFrame::onGrantSlot(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	uint64_t time = 0;
	switch(wID) {
		case IDC_GRANTSLOT:			time = 600; break;
		case IDC_GRANTSLOT_DAY:		time = 3600; break;
		case IDC_GRANTSLOT_HOUR:	time = 24*3600; break;
		case IDC_GRANTSLOT_WEEK:	time = 7*24*3600; break;
		case IDC_UNGRANTSLOT:		time = 0; break;
	}
	
	if(time > 0)
		UploadManager::getInstance()->reserveSlot(HintedUser(replyTo, replyTo.hint), time);
	else
		UploadManager::getInstance()->unreserveSlot(replyTo);

	return 0;
}

LRESULT PrivateFrame::onAddToFavorites(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	FavoriteManager::getInstance()->addFavoriteUser(replyTo);
	return 0;
}

void PrivateFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);
	
	if(ctrlStatus.IsWindow()) {
		CRect sr;
		int w[1];
		ctrlStatus.GetClientRect(sr);
		
		w[0] = sr.right - 16;

		ctrlStatus.SetParts(1, w);
	}
	
	int h = WinUtil::fontHeight + 4;

	CRect rc = rect;
	rc.bottom -= (2*h) + 10;
	ctrlClient.MoveWindow(rc);
	
	rc = rect;
	rc.bottom -= 2;
	rc.top = rc.bottom - (2*h) - 5;

	rc.right -= rc.Height()+2;
	ctrlMessage.MoveWindow(rc);
	
	rc.left = rc.right + 2;
  	rc.right += rc.Height();
  	 
  	ctrlEmoticons.MoveWindow(rc);
}

void PrivateFrame::updateTitle() {
	pair<tstring, bool> hubs = WinUtil::getHubNames(replyTo.user->getCID(), replyTo.hint, priv);
	if(hubs.second) {	
		unsetIconState();
		setTabColor(RGB(0, 255,	255));
		if(isoffline) {
			tstring status = _T(" *** ") + TSTRING(USER_WENT_ONLINE) + _T(" [") + WinUtil::getNicks(replyTo.user->getCID(), replyTo.hint, priv) + _T(" - ") + hubs.first + _T("] ***");
			if(BOOLSETTING(STATUS_IN_CHAT)) {
				addLine(status, WinUtil::m_ChatTextServer);
			} else {
				addClientLine(status);
			}
		}
		isoffline = false;
	} else {
		setIconState();
		setTabColor(RGB(255, 0, 0));
		tstring status = _T(" *** ") + TSTRING(USER_WENT_OFFLINE);
		if(BOOLSETTING(STATUS_IN_CHAT)) {
			addLine(status, WinUtil::m_ChatTextServer);
		} else {
			addClientLine(status);
		}
		isoffline = true;
		ctrlClient.setClient(NULL);
	}
	SetWindowText((WinUtil::getNicks(replyTo.user->getCID(), replyTo.hint, priv) + _T(" - ") + hubs.first).c_str());
}

LRESULT PrivateFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	if(reinterpret_cast<HWND>(wParam) == ctrlEmoticons) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click
		menuItems = 0;

		if(emoMenu != NULL)
			emoMenu.DestroyMenu();

		emoMenu.CreatePopupMenu();
		emoMenu.InsertSeparatorFirst(_T("Emoticons Pack"));
		emoMenu.AppendMenu(MF_STRING, IDC_EMOMENU, _T("Disabled"));
		
		if (SETTING(EMOTICONS_FILE) == "Disabled")
			emoMenu.CheckMenuItem( IDC_EMOMENU, MF_BYCOMMAND | MF_CHECKED );
		
		// nacteme seznam emoticon packu (vsechny *.xml v adresari EmoPacks)
		WIN32_FIND_DATA data;
		HANDLE hFind;
		hFind = FindFirstFile(Text::toT(Util::getPath(Util::PATH_EMOPACKS) + "*.xml").c_str(), &data);
		if(hFind != INVALID_HANDLE_VALUE) {
			do {
				tstring name = data.cFileName;
				tstring::size_type i = name.rfind('.');
				name = name.substr(0, i);

				menuItems++;
				emoMenu.AppendMenu(MF_STRING, IDC_EMOMENU + menuItems, name.c_str());
				if(name == Text::toT(SETTING(EMOTICONS_FILE))) emoMenu.CheckMenuItem( IDC_EMOMENU + menuItems, MF_BYCOMMAND | MF_CHECKED );
			} while(FindNextFile(hFind, &data));
			FindClose(hFind);
		}
		
		if(menuItems>0)
			emoMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);

		emoMenu.RemoveFirstItem();
	}

	return 0;
}

void PrivateFrame::readLog() {
	if (SETTING(SHOW_LAST_LINES_LOG) == 0) return;

	StringMap params;
	
	const CID& cid = replyTo.user->getCID();
	const string& hint = replyTo.hint;
					
	params["hubNI"] = Util::toString(ClientManager::getInstance()->getHubNames(cid, hint, priv));
	params["hubURL"] = Util::toString(ClientManager::getInstance()->getHubs(cid, hint, priv));
	params["userCID"] = cid.toBase32(); 
	params["userNI"] = ClientManager::getInstance()->getNicks(cid, hint, priv)[0];
	params["myCID"] = ClientManager::getInstance()->getMe()->getCID().toBase32();
		
	string path = Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatParams(SETTING(LOG_FILE_PRIVATE_CHAT), params, false));

	try {
		File f(path, File::READ, File::OPEN);
		
		int64_t size = f.getSize();

		if(size > 32*1024) {
			f.setPos(size - 32*1024);
		}
		string buf = f.read(32*1024);
		StringList lines;

		if(strnicmp(buf.c_str(), "\xef\xbb\xbf", 3) == 0)
			lines = StringTokenizer<string>(buf.substr(3), "\r\n").getTokens();
		else
			lines = StringTokenizer<string>(buf, "\r\n").getTokens();

		int linesCount = lines.size();

		int i = linesCount > (SETTING(SHOW_LAST_LINES_LOG) + 1) ? linesCount - SETTING(SHOW_LAST_LINES_LOG) : 0;

		for(; i < linesCount; ++i){
			ctrlClient.AppendText(Identity(NULL, 0), _T("- "), _T(""), Text::toT(lines[i]) + _T('\n'), WinUtil::m_ChatTextLog, true);
		}

		f.close();
	} catch(const FileException&){
	}
}

void PrivateFrame::closeAll(){
	for(FrameIter i = frames.begin(); i != frames.end(); ++i)
		i->second->PostMessage(WM_CLOSE, 0, 0);
}

void PrivateFrame::closeAllOffline() {
	for(FrameIter i = frames.begin(); i != frames.end(); ++i) {
		if(!i->first->isOnline())
			i->second->PostMessage(WM_CLOSE, 0, 0);
	}
}

LRESULT PrivateFrame::onOpenUserLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {	
	StringMap params;
	const CID& cid = replyTo.user->getCID();
	const string& hint = replyTo.hint;
					
	params["hubNI"] = Util::toString(ClientManager::getInstance()->getHubNames(cid, hint, priv));
	params["hubURL"] = Util::toString(ClientManager::getInstance()->getHubs(cid, hint, priv));
	params["userCID"] = cid.toBase32(); 
	params["userNI"] = ClientManager::getInstance()->getNicks(cid, hint, priv)[0];
	params["myCID"] = ClientManager::getInstance()->getMe()->getCID().toBase32();

	string file = Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatParams(SETTING(LOG_FILE_PRIVATE_CHAT), params, false));
	if(Util::fileExists(file)) {
		ShellExecute(NULL, NULL, Text::toT(file).c_str(), NULL, NULL, SW_SHOWNORMAL);
	} else {
		MessageBox(CTSTRING(NO_LOG_FOR_USER), CTSTRING(NO_LOG_FOR_USER), MB_OK );	  
	}	

	return 0;
}

void PrivateFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw() {
	ctrlClient.SetBackgroundColor(WinUtil::bgColor);
	RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
}

LRESULT PrivateFrame::onEmoPackChange(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	TCHAR buf[256];
	emoMenu.GetMenuString(wID, buf, 256, MF_BYCOMMAND);
	if (buf!=Text::toT(SETTING(EMOTICONS_FILE))) {
		SettingsManager::getInstance()->set(SettingsManager::EMOTICONS_FILE, Text::fromT(buf));
		emoticonsManager->Unload();
		emoticonsManager->Load();
	}
	return 0;
}

LRESULT PrivateFrame::onEmoticons(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& bHandled) {
	if (hWndCtl != ctrlEmoticons.m_hWnd) {
		bHandled = false;
        return 0;
    }
 
	EmoticonsDlg dlg;
	ctrlEmoticons.GetWindowRect(dlg.pos);
	dlg.DoModal(m_hWnd);
	if (!dlg.result.empty()) {
		TCHAR* message = new TCHAR[ctrlMessage.GetWindowTextLength()+1];
		ctrlMessage.GetWindowText(message, ctrlMessage.GetWindowTextLength()+1);
		tstring s(message, ctrlMessage.GetWindowTextLength());
		delete[] message;
		
		ctrlMessage.SetWindowText((s+dlg.result).c_str());
		ctrlMessage.SetFocus();
		ctrlMessage.SetSel( ctrlMessage.GetWindowTextLength(), ctrlMessage.GetWindowTextLength() );
	}
	return 0;
}

LRESULT PrivateFrame::onPublicMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	if(isoffline)
		return 0;

	tstring sUsers = ctrlClient.getSelectedUser();

	int iSelBegin, iSelEnd;
	ctrlMessage.GetSel( iSelBegin, iSelEnd );

	if ( ( iSelBegin == 0 ) && ( iSelEnd == 0 ) ) {
		sUsers += _T(": ");
		if (ctrlMessage.GetWindowTextLength() == 0) {	
			ctrlMessage.SetWindowText(sUsers.c_str());
			ctrlMessage.SetFocus();
			ctrlMessage.SetSel( ctrlMessage.GetWindowTextLength(), ctrlMessage.GetWindowTextLength() );
		} else {
			ctrlMessage.ReplaceSel( sUsers.c_str() );
			ctrlMessage.SetFocus();
		}
	} else {
		sUsers += _T(" ");
		ctrlMessage.ReplaceSel( sUsers.c_str() );
		ctrlMessage.SetFocus();
	}
	return 0;
}

/**
 * @file
 * $Id$
 */
