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

#include "QueueFrame.h"
#include "SearchFrm.h"
#include "PrivateFrame.h"
#include "LineDlg.h"

#include "../client/StringTokenizer.h"
#include "../client/ShareManager.h"
#include "../client/ClientManager.h"
#include "../client/version.h"
#include "BarShader.h"

#define FILE_LIST_NAME _T("File Lists")

int QueueFrame::columnIndexes[] = { COLUMN_TARGET, COLUMN_STATUS, COLUMN_SEGMENTS, COLUMN_SIZE, COLUMN_PROGRESS, COLUMN_DOWNLOADED, COLUMN_PRIORITY,
COLUMN_USERS, COLUMN_PATH, COLUMN_EXACT_SIZE, COLUMN_ERRORS, COLUMN_ADDED, COLUMN_TTH };

int QueueFrame::columnSizes[] = { 200, 300, 70, 75, 100, 120, 75, 200, 200, 75, 200, 100, 125 };

static ResourceManager::Strings columnNames[] = { ResourceManager::FILENAME, ResourceManager::STATUS, ResourceManager::SEGMENTS, ResourceManager::SIZE, 
ResourceManager::DOWNLOADED_PARTS, ResourceManager::DOWNLOADED,
ResourceManager::PRIORITY, ResourceManager::USERS, ResourceManager::PATH, ResourceManager::EXACT_SIZE, ResourceManager::ERRORS,
ResourceManager::ADDED, ResourceManager::TTH_ROOT };

LRESULT QueueFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	showTree = BOOLSETTING(QUEUEFRAME_SHOW_TREE);
	
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);
	
	ctrlQueue.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_QUEUE);
	ctrlQueue.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);

	ctrlDirs.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
		TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES | TVS_SHOWSELALWAYS | TVS_DISABLEDRAGDROP | TVS_TRACKSELECT, 
		 WS_EX_CLIENTEDGE, IDC_DIRECTORIES);
	
	if(BOOLSETTING(USE_EXPLORER_THEME) &&
		((WinUtil::getOsMajor() >= 5 && WinUtil::getOsMinor() >= 1) //WinXP & WinSvr2003
		|| (WinUtil::getOsMajor() >= 6))) //Vista & Win7
	{
		SetWindowTheme(ctrlDirs.m_hWnd, L"explorer", NULL);
	}

	ctrlDirs.SetImageList(WinUtil::fileImages, TVSIL_NORMAL);
	ctrlQueue.SetImageList(WinUtil::fileImages, LVSIL_SMALL);
	
	m_nProportionalPos = 2500;
	SetSplitterPanes(ctrlDirs.m_hWnd, ctrlQueue.m_hWnd);

	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(QUEUEFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(QUEUEFRAME_WIDTHS), COLUMN_LAST);
	
	for(uint8_t j=0; j<COLUMN_LAST; j++) {
		int fmt = (j == COLUMN_SIZE || j == COLUMN_DOWNLOADED || j == COLUMN_EXACT_SIZE|| j == COLUMN_SEGMENTS) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlQueue.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}
	
	ctrlQueue.setColumnOrderArray(COLUMN_LAST, columnIndexes);
	ctrlQueue.setSortColumn(COLUMN_TARGET);
	ctrlQueue.setVisible(SETTING(QUEUEFRAME_VISIBLE));
	
	ctrlQueue.SetBkColor(WinUtil::bgColor);
	ctrlQueue.SetTextBkColor(WinUtil::bgColor);
	ctrlQueue.SetTextColor(WinUtil::textColor);
	ctrlQueue.setFlickerFree(WinUtil::bgBrush);

	ctrlDirs.SetBkColor(WinUtil::bgColor);
	ctrlDirs.SetTextColor(WinUtil::textColor);

	ctrlShowTree.Create(ctrlStatus.m_hWnd, rcDefault, _T("+/-"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	ctrlShowTree.SetButtonStyle(BS_AUTOCHECKBOX, false);
	ctrlShowTree.SetCheck(showTree);
	showTreeContainer.SubclassWindow(ctrlShowTree.m_hWnd);
	
	browseMenu.CreatePopupMenu();
	removeMenu.CreatePopupMenu();
	removeAllMenu.CreatePopupMenu();
	pmMenu.CreatePopupMenu();
	readdMenu.CreatePopupMenu();

	removeMenu.AppendMenu(MF_STRING, IDC_REMOVE_SOURCE, CTSTRING(ALL));
	removeMenu.AppendMenu(MF_SEPARATOR);

	readdMenu.AppendMenu(MF_STRING, IDC_READD, CTSTRING(ALL));
	readdMenu.AppendMenu(MF_SEPARATOR);

	addQueueList(QueueManager::getInstance()->lockQueue());
	QueueManager::getInstance()->unlockQueue();
	QueueManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);

	memzero(statusSizes, sizeof(statusSizes));
	statusSizes[0] = 16;
	ctrlStatus.SetParts(6, statusSizes);
	updateStatus();

	bHandled = FALSE;
	return 1;
}

const tstring QueueFrame::QueueItemInfo::getText(int col) const {
	switch(col) {
		case COLUMN_TARGET: return Text::toT(Util::getFileName(getTarget()));
		case COLUMN_STATUS: {
			int online = 0;
			QueueItem::SourceList sources = QueueManager::getInstance()->getSources(qi);
			for(QueueItem::SourceConstIter j = sources.begin(); j != sources.end(); ++j) {
				if(j->getUser().user->isOnline())
					online++;
			}

			if(isFinished()) {
				return TSTRING(DOWNLOAD_FINISHED_IDLE);
			} else if(isWaiting()) {
				if(online > 0) {
					size_t size = QueueManager::getInstance()->getSourcesCount(qi);
					if(size == 1) {
						return TSTRING(WAITING_USER_ONLINE);
					} else {
						TCHAR buf[64];
						_stprintf(buf, CTSTRING(WAITING_USERS_ONLINE), online, size);
						return buf;
					}
				} else {
					size_t size = QueueManager::getInstance()->getSourcesCount(qi);
					if(size == 0) {
						return TSTRING(NO_USERS_TO_DOWNLOAD_FROM);
					} else if(size == 1) {
						return TSTRING(USER_OFFLINE);
					} else if(size == 2) {
						return TSTRING(BOTH_USERS_OFFLINE);
					} else if(size == 3) {
						return TSTRING(ALL_3_USERS_OFFLINE);
					} else if(size == 4) {
						return TSTRING(ALL_4_USERS_OFFLINE);
					} else {
						TCHAR buf[64];
						_stprintf(buf, CTSTRING(ALL_USERS_OFFLINE), size);
						return buf;
					}
				}
			} else {
				size_t size = QueueManager::getInstance()->getSourcesCount(qi);
				if(size == 1) {
					return TSTRING(USER_ONLINE);
				} else {
					TCHAR buf[64];
					_stprintf(buf, CTSTRING(USERS_ONLINE), online, size);
					return buf;
				}
			}
		}
		case COLUMN_SEGMENTS: {
			const QueueItem* qi = QueueManager::getInstance()->fileQueue.find(getTarget());
			return Util::toStringW(qi ? qi->getDownloads().size() : 0) + _T("/") + Util::toStringW(qi ? qi->getMaxSegments() : 0);
		}
		case COLUMN_SIZE: return (getSize() == -1) ? TSTRING(UNKNOWN) : Util::formatBytesW(getSize());
		case COLUMN_DOWNLOADED: {
			int64_t downloadedBytes = getDownloadedBytes();
			return (getSize() > 0) ? Util::formatBytesW(downloadedBytes) + _T(" (") + Util::toStringW((double)downloadedBytes*100.0/(double)getSize()) + _T("%)") : Util::emptyStringT;
		}
		case COLUMN_PRIORITY: {
			tstring priority;
			switch(getPriority()) {
				case QueueItem::PAUSED: priority = TSTRING(PAUSED); break;
				case QueueItem::LOWEST: priority = TSTRING(LOWEST); break;
				case QueueItem::LOW: priority = TSTRING(LOW); break;
				case QueueItem::NORMAL: priority = TSTRING(NORMAL); break;
				case QueueItem::HIGH: priority = TSTRING(HIGH); break;
				case QueueItem::HIGHEST: priority = TSTRING(HIGHEST); break;
				default: dcassert(0); break;
			}
			if(getAutoPriority()) {
				priority += _T(" (") + TSTRING(AUTO) + _T(")");
			}
			return priority;
		}
		case COLUMN_USERS: {
			tstring tmp;

			QueueItem::SourceList sources = QueueManager::getInstance()->getSources(qi);
			for(QueueItem::SourceConstIter j = sources.begin(); j != sources.end(); ++j) {
				if(tmp.size() > 0)
					tmp += _T(", ");

				tmp += WinUtil::getNicks(j->getUser());
			}
			return tmp.empty() ? TSTRING(NO_USERS) : tmp;
		}
		case COLUMN_PATH: return Text::toT(getPath());
		case COLUMN_EXACT_SIZE: return (getSize() == -1) ? TSTRING(UNKNOWN) : Util::formatExactSize(getSize());
		case COLUMN_ERRORS: {
			tstring tmp;
			QueueItem::SourceList badSources = QueueManager::getInstance()->getBadSources(qi);
			for(QueueItem::SourceConstIter j = badSources.begin(); j != badSources.end(); ++j) {
				if(!j->isSet(QueueItem::Source::FLAG_REMOVED)) {
				if(tmp.size() > 0)
					tmp += _T(", ");
					tmp += WinUtil::getNicks(j->getUser());
					tmp += _T(" (");
					if(j->isSet(QueueItem::Source::FLAG_FILE_NOT_AVAILABLE)) {
						tmp += TSTRING(FILE_NOT_AVAILABLE);
					} else if(j->isSet(QueueItem::Source::FLAG_PASSIVE)) {
						tmp += TSTRING(PASSIVE_USER);
					} else if(j->isSet(QueueItem::Source::FLAG_BAD_TREE)) {
						tmp += TSTRING(INVALID_TREE);
					} else if(j->isSet(QueueItem::Source::FLAG_SLOW_SOURCE)) {
						tmp += TSTRING(SLOW_USER);
					} else if(j->isSet(QueueItem::Source::FLAG_NO_TTHF)) {
						tmp += TSTRING(SOURCE_TOO_OLD);						
					} else if(j->isSet(QueueItem::Source::FLAG_NO_NEED_PARTS)) {
						tmp += TSTRING(NO_NEEDED_PART);
					} else if(j->isSet(QueueItem::Source::FLAG_UNTRUSTED)) {
						tmp += TSTRING(CERTIFICATE_NOT_TRUSTED);
					}
					tmp += _T(')');
				}
			}
			return tmp.empty() ? TSTRING(NO_ERRORS) : tmp;
		}
		case COLUMN_ADDED: return Text::toT(Util::formatTime("%Y-%m-%d %H:%M", getAdded()));
		case COLUMN_TTH: 
			return qi->isSet(QueueItem::FLAG_USER_LIST) ? Util::emptyStringT : Text::toT(getTTH().toBase32());

		default: return Util::emptyStringT;
	}
}

void QueueFrame::on(QueueManagerListener::Added, QueueItem* aQI) {
	QueueItemInfo* ii = new QueueItemInfo(aQI);

	speak(ADD_ITEM,	new QueueItemInfoTask(ii));
}

void QueueFrame::addQueueItem(QueueItemInfo* ii, bool noSort) {
	if(!ii->isSet(QueueItem::FLAG_USER_LIST)) {
		queueSize+=ii->getSize();
	}
	queueItems++;
	dirty = true;
	
	const string& dir = ii->getPath();
	
	bool updateDir = (directories.find(dir) == directories.end());
	directories.insert(make_pair(dir, ii));
	
	if(updateDir) {
		addDirectory(dir, ii->isSet(QueueItem::FLAG_USER_LIST));
	} 
	if(!showTree || isCurDir(dir)) {
		if(noSort)
			ctrlQueue.insertItem(ctrlQueue.GetItemCount(), ii, WinUtil::getIconIndex(Text::toT(ii->getTarget())));
		else
			ctrlQueue.insertItem(ii, WinUtil::getIconIndex(Text::toT(ii->getTarget())));
	}
}

QueueFrame::QueueItemInfo* QueueFrame::getItemInfo(const string& target) const {
	string path = Util::getFilePath(target);
	DirectoryPairC items = directories.equal_range(path);
	for(DirectoryIterC i = items.first; i != items.second; ++i) {
		if(i->second->getTarget() == target) {
			return i->second;
		}
	}
	return 0;
}

void QueueFrame::addQueueList(const QueueItem::StringMap& li) {
	ctrlQueue.SetRedraw(FALSE);
	ctrlDirs.SetRedraw(FALSE);
	for(QueueItem::StringMap::const_iterator j = li.begin(); j != li.end(); ++j) {
		QueueItem* aQI = j->second;
		QueueItemInfo* ii = new QueueItemInfo(aQI);
		addQueueItem(ii, true);
	}
	ctrlQueue.resort();
	ctrlQueue.SetRedraw(TRUE);
	ctrlDirs.SetRedraw(TRUE);
	ctrlDirs.Invalidate();
}

HTREEITEM QueueFrame::addDirectory(const string& dir, bool isFileList /* = false */, HTREEITEM startAt /* = NULL */) {
	TVINSERTSTRUCT tvi;
	tvi.hInsertAfter = TVI_SORT;
	tvi.item.mask = TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_TEXT;
	tvi.item.iImage = tvi.item.iSelectedImage = WinUtil::getDirIconIndex();

	if(BOOLSETTING(EXPAND_QUEUE)) {
		tvi.itemex.mask |= TVIF_STATE;
		tvi.itemex.state = TVIS_EXPANDED;
		tvi.itemex.stateMask = TVIS_EXPANDED;
	}

	if(isFileList) {
		// We assume we haven't added it yet, and that all filelists go to the same
		// directory...
		dcassert(fileLists == NULL);
		tvi.hParent = NULL;
		tvi.item.pszText = FILE_LIST_NAME;
		tvi.item.lParam = (LPARAM) new string(dir);
		fileLists = ctrlDirs.InsertItem(&tvi);
		return fileLists;
	} 

	// More complicated, we have to find the last available tree item and then see...
	string::size_type i = 0;
	string::size_type j;

	HTREEITEM next = NULL;
	HTREEITEM parent = NULL;

	if(startAt == NULL) {
		// First find the correct drive letter
		dcassert(dir[1] == ':');
		dcassert(dir[2] == '\\');

		next = ctrlDirs.GetRootItem();

		while(next != NULL) {
			if(next != fileLists) {
				string* stmp = reinterpret_cast<string*>(ctrlDirs.GetItemData(next));
				if(strnicmp(*stmp, dir, 3) == 0)
					break;
			}
			next = ctrlDirs.GetNextSiblingItem(next);
		}

		if(next == NULL) {
			// First addition, set commonStart to the dir minus the last part...
			i = dir.rfind('\\', dir.length()-2);
			if(i != string::npos) {
				tstring name = Text::toT(dir.substr(0, i));
				tvi.hParent = NULL;
				tvi.item.pszText = const_cast<TCHAR*>(name.c_str());
				tvi.item.lParam = (LPARAM)new string(dir.substr(0, i+1));
				next = ctrlDirs.InsertItem(&tvi);
			} else {
				dcassert(dir.length() == 3);
				tstring name = Text::toT(dir);
				tvi.hParent = NULL;
				tvi.item.pszText = const_cast<TCHAR*>(name.c_str());
				tvi.item.lParam = (LPARAM)new string(dir);
				next = ctrlDirs.InsertItem(&tvi);
			}
		} 

		// Ok, next now points to x:\... find how much is common

		string* rootStr = (string*)ctrlDirs.GetItemData(next);

		i = 0;

		for(;;) {
			j = dir.find('\\', i);
			if(j == string::npos)
				break;
			if(strnicmp(dir.c_str() + i, rootStr->c_str() + i, j - i + 1) != 0)
				break;
			i = j + 1;
		}

		if(i < rootStr->length()) {
			HTREEITEM oldRoot = next;

			// Create a new root
			tstring name = Text::toT(rootStr->substr(0, i-1));
			tvi.hParent = NULL;
			tvi.item.pszText = const_cast<TCHAR*>(name.c_str());
			tvi.item.lParam = (LPARAM)new string(rootStr->substr(0, i));
			HTREEITEM newRoot = ctrlDirs.InsertItem(&tvi);

			parent = addDirectory(*rootStr, false, newRoot);

			next = ctrlDirs.GetChildItem(oldRoot);
			while(next != NULL) {
				moveNode(next, parent);
				next = ctrlDirs.GetChildItem(oldRoot);
			}
			delete rootStr;
			ctrlDirs.DeleteItem(oldRoot);
			parent = newRoot;
		} else {
			// Use this root as parent
			parent = next;
			next = ctrlDirs.GetChildItem(parent);
		}
	} else {
		parent = startAt;
		next = ctrlDirs.GetChildItem(parent);
		i = getDir(parent).length();
		dcassert(strnicmp(getDir(parent), dir, getDir(parent).length()) == 0);
	}

	while( i < dir.length() ) {
		while(next != NULL) {
			if(next != fileLists) {
				const string& n = getDir(next);
				if(strnicmp(n.c_str()+i, dir.c_str()+i, n.length()-i) == 0) {
					// Found a part, we assume it's the best one we can find...
					i = n.length();

					parent = next;
					next = ctrlDirs.GetChildItem(next);
					break;
				}
			}
			next = ctrlDirs.GetNextSiblingItem(next);
		}

		if(next == NULL) {
			// We didn't find it, add...
			j = dir.find('\\', i);
			dcassert(j != string::npos);
			tstring name = Text::toT(dir.substr(i, j-i));
			tvi.hParent = parent;
			tvi.item.pszText = const_cast<TCHAR*>(name.c_str());
			tvi.item.lParam = (LPARAM) new string(dir.substr(0, j+1));

			parent = ctrlDirs.InsertItem(&tvi);

			i = j + 1;
		}
	}

	return parent;
}

void QueueFrame::removeDirectory(const string& dir, bool isFileList /* = false */) {

	// First, find the last name available
	string::size_type i = 0;

	HTREEITEM next = ctrlDirs.GetRootItem();
	HTREEITEM parent = NULL;
	
	if(isFileList) {
		dcassert(fileLists != NULL);
		delete (string*)ctrlDirs.GetItemData(fileLists);
		ctrlDirs.DeleteItem(fileLists);
		fileLists = NULL;
		return;
	} else {
		while(i < dir.length()) {
			while(next != NULL) {
				if(next != fileLists) {
				const string& n = getDir(next);
				if(strnicmp(n.c_str()+i, dir.c_str()+i, n.length()-i) == 0) {
					// Match!
					parent = next;
					next = ctrlDirs.GetChildItem(next);
					i = n.length();
					break;
				}
				}
				next = ctrlDirs.GetNextSiblingItem(next);
			}
			if(next == NULL)
				break;
		}
	}

	next = parent;

	while((ctrlDirs.GetChildItem(next) == NULL) && (directories.find(getDir(next)) == directories.end())) {
		delete (string*)ctrlDirs.GetItemData(next);
		parent = ctrlDirs.GetParentItem(next);
		
		ctrlDirs.DeleteItem(next);
		if(parent == NULL)
			break;
		next = parent;
	}
}

void QueueFrame::removeDirectories(HTREEITEM ht) {
	HTREEITEM next = ctrlDirs.GetChildItem(ht);
	while(next != NULL) {
		removeDirectories(next);
		next = ctrlDirs.GetNextSiblingItem(ht);
	}
	delete (string*)ctrlDirs.GetItemData(ht);
	ctrlDirs.DeleteItem(ht);
}

void QueueFrame::on(QueueManagerListener::Removed, const QueueItem* aQI) {
	speak(REMOVE_ITEM, new StringTask(aQI->getTarget()));
}

void QueueFrame::on(QueueManagerListener::Moved, const QueueItem*, const string& oldTarget) {
	speak(REMOVE_ITEM, new StringTask(oldTarget));
	
	// we need to call speaker now to properly remove item before other actions
	onSpeaker(0, 0, 0, *reinterpret_cast<BOOL*>(NULL));
}

void QueueFrame::on(QueueManagerListener::SourcesUpdated, const QueueItem* aQI) {
	speak(UPDATE_ITEM, new UpdateTask(*aQI));
}

LRESULT QueueFrame::onSpeaker(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	TaskQueue::List t;

	tasks.get(t);
	spoken = false;

	for(TaskQueue::Iter ti = t.begin(); ti != t.end(); ++ti) {
		if(ti->first == ADD_ITEM) {
			auto_ptr<QueueItemInfoTask> iit(static_cast<QueueItemInfoTask*>(ti->second));
			
			dcassert(ctrlQueue.findItem(iit->ii) == -1);
			addQueueItem(iit->ii, false);
			updateStatus();
		} else if(ti->first == REMOVE_ITEM) {
			auto_ptr<StringTask> target(static_cast<StringTask*>(ti->second));
			const QueueItemInfo* ii = getItemInfo(target->str);
			if(!ii) {
				dcassert(ii);
				continue;
			}
			
			if(!showTree || isCurDir(ii->getPath()) ) {
				dcassert(ctrlQueue.findItem(ii) != -1);
				ctrlQueue.deleteItem(ii);
			}
			
			bool userList = ii->isSet(QueueItem::FLAG_USER_LIST);
			
			if(!userList) {
				queueSize-=ii->getSize();
				dcassert(queueSize >= 0);
			}
			queueItems--;
			dcassert(queueItems >= 0);
			
			pair<DirectoryIter, DirectoryIter> i = directories.equal_range(ii->getPath());
			DirectoryIter j;
			for(j = i.first; j != i.second; ++j) {
				if(j->second == ii)
					break;
			}
			dcassert(j != i.second);
			directories.erase(j);
			if(directories.count(ii->getPath()) == 0) {
				removeDirectory(ii->getPath(), ii->isSet(QueueItem::FLAG_USER_LIST));
				if(isCurDir(ii->getPath()))
					curDir.clear();
			}
			
			delete ii;
			updateStatus();
			if (!userList && BOOLSETTING(BOLD_QUEUE)) {
				setDirty();
			}
			dirty = true;
		} else if(ti->first == UPDATE_ITEM) {
			auto_ptr<UpdateTask> ui(reinterpret_cast<UpdateTask*>(ti->second));
            QueueItemInfo* ii = getItemInfo(ui->target);
			if(!ii)
				continue;

			if(!showTree || isCurDir(ii->getPath())) {
				int pos = ctrlQueue.findItem(ii);
				if(pos != -1) {
					ctrlQueue.updateItem(pos, COLUMN_SEGMENTS);
					ctrlQueue.updateItem(pos, COLUMN_PROGRESS);
					ctrlQueue.updateItem(pos, COLUMN_PRIORITY);
					ctrlQueue.updateItem(pos, COLUMN_USERS);
					ctrlQueue.updateItem(pos, COLUMN_ERRORS);
					ctrlQueue.updateItem(pos, COLUMN_STATUS);
					ctrlQueue.updateItem(pos, COLUMN_DOWNLOADED);
				}
			}
		} else if(ti->first == UPDATE_STATUS) {
			auto_ptr<StringTask> status(static_cast<StringTask*>(ti->second));
			ctrlStatus.SetText(1, Text::toT(status->str).c_str());
		}
	}

	return 0;
}

void QueueFrame::removeSelected() {
	if(!BOOLSETTING(CONFIRM_DELETE) || MessageBox(CTSTRING(REALLY_REMOVE), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)
		ctrlQueue.forEachSelected(&QueueItemInfo::remove);
}
	
void QueueFrame::removeSelectedDir() { 
	if(!BOOLSETTING(CONFIRM_DELETE) || MessageBox(CTSTRING(REALLY_REMOVE), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)	
		removeDir(ctrlDirs.GetSelectedItem()); 
}

void QueueFrame::moveSelected() {

	int n = ctrlQueue.GetSelectedCount();
	if(n == 1) {
		// Single file, get the full filename and move...
		const QueueItemInfo* ii = ctrlQueue.getItemData(ctrlQueue.GetNextItem(-1, LVNI_SELECTED));
		tstring target = Text::toT(ii->getTarget());
		tstring ext = Util::getFileExt(target);
		tstring ext2;
		if (!ext.empty())
		{
			ext = ext.substr(1); // remove leading dot so default extension works when browsing for file
			ext2 = _T("*.") + ext;
			ext2 += (TCHAR)0;
			ext2 += _T("*.") + ext;
		}
		ext2 += _T("*.*\0*.*\0\0");

		tstring path = Text::toT(ii->getPath());
		if(WinUtil::browseFile(target, m_hWnd, true, path, ext2.c_str(), ext.empty() ? NULL : ext.c_str())) {
			QueueManager::getInstance()->move(ii->getTarget(), Text::fromT(target));
		}
	} else if(n > 1) {
		tstring name;
		if(showTree) {
			name = Text::toT(curDir);
		}

		if(WinUtil::browseDirectory(name, m_hWnd)) {
			int i = -1;
			while( (i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1) {
				const QueueItemInfo* ii = ctrlQueue.getItemData(i);
				QueueManager::getInstance()->move(ii->getTarget(), Text::fromT(name) + Util::getFileName(ii->getTarget()));
			}			
		}
	}
}

void QueueFrame::moveSelectedDir() {
	if(ctrlDirs.GetSelectedItem() == NULL)
		return;

	dcassert(!curDir.empty());
	tstring name = Text::toT(curDir);
	
	if(WinUtil::browseDirectory(name, m_hWnd)) {
		tmp.clear();
		moveDir(ctrlDirs.GetSelectedItem(), Text::fromT(name));

		for(vector<pair<QueueItemInfo*, string>>::const_iterator i = tmp.begin(); i != tmp.end(); ++i) {
			QueueManager::getInstance()->move((*i).first->getTarget(), (*i).second + Util::getFileName((*i).first->getTarget()));
		}

		tmp.clear();
	}
}

void QueueFrame::moveDir(HTREEITEM ht, const string& target) {

	HTREEITEM next = ctrlDirs.GetChildItem(ht);
	while(next != NULL) {
		// must add path separator since getLastDir only give us the name
		moveDir(next, target + Util::getLastDir(getDir(next)) + PATH_SEPARATOR);
		next = ctrlDirs.GetNextSiblingItem(next);
	}

	string* s = (string*)ctrlDirs.GetItemData(ht);

	DirectoryPairC p = directories.equal_range(*s);

	for(DirectoryIterC i = p.first; i != p.second; ++i) {
		tmp.push_back(make_pair(i->second, target));
	}
}

QueueItem::SourceList sources;
QueueItem::SourceList badSources;
LRESULT QueueFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {

	OMenu priorityMenu;
	priorityMenu.CreatePopupMenu();
	priorityMenu.InsertSeparatorFirst(TSTRING(PRIORITY));
	priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_PAUSED, CTSTRING(PAUSED));
	priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_LOWEST, CTSTRING(LOWEST));
	priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_LOW, CTSTRING(LOW));
	priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_NORMAL, CTSTRING(NORMAL));
	priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_HIGH, CTSTRING(HIGH));
	priorityMenu.AppendMenu(MF_STRING, IDC_PRIORITY_HIGHEST, CTSTRING(HIGHEST));
	priorityMenu.AppendMenu(MF_STRING, IDC_AUTOPRIORITY, CTSTRING(AUTO));

	if (reinterpret_cast<HWND>(wParam) == ctrlQueue && ctrlQueue.GetSelectedCount() > 0) { 
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	
		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlQueue, pt);
		}

		OMenu segmentsMenu;
		segmentsMenu.CreatePopupMenu();
		segmentsMenu.InsertSeparatorFirst(TSTRING(MAX_SEGMENTS_NUMBER));
		for(int i = IDC_SEGMENTONE; i <= IDC_SEGMENTTEN; i++)
			segmentsMenu.AppendMenu(MF_STRING, i, (Util::toStringW(i - 109) + _T(" ") + TSTRING(SEGMENTS)).c_str());
			
		if(ctrlQueue.GetSelectedCount() > 0) { 
			usingDirMenu = false;
			CMenuItemInfo mi;
		
			while(browseMenu.GetMenuItemCount() > 0) {
				browseMenu.RemoveMenu(0, MF_BYPOSITION);
			}
			while(removeMenu.GetMenuItemCount() > 2) {
				removeMenu.RemoveMenu(2, MF_BYPOSITION);
			}
			while(removeAllMenu.GetMenuItemCount() > 0) {
				removeAllMenu.RemoveMenu(0, MF_BYPOSITION);
			}
			while(pmMenu.GetMenuItemCount() > 0) {
				pmMenu.RemoveMenu(0, MF_BYPOSITION);
			}
			while(readdMenu.GetMenuItemCount() > 2) {
				readdMenu.RemoveMenu(2, MF_BYPOSITION);
			}

			if(ctrlQueue.GetSelectedCount() == 1) {
				OMenu copyMenu;
				copyMenu.CreatePopupMenu();
				copyMenu.InsertSeparatorFirst(TSTRING(COPY));
				copyMenu.AppendMenu(MF_STRING, IDC_COPY_LINK, CTSTRING(COPY_MAGNET_LINK));
				for(int i = 0; i <COLUMN_LAST; ++i)
					copyMenu.AppendMenu(MF_STRING, IDC_COPY + i, CTSTRING_I(columnNames[i]));

				OMenu previewMenu;
				previewMenu.CreatePopupMenu();
				previewMenu.InsertSeparatorFirst(TSTRING(PREVIEW_MENU));

				OMenu singleMenu;
				singleMenu.CreatePopupMenu();
				singleMenu.InsertSeparatorFirst(TSTRING(FILE));
				singleMenu.AppendMenu(MF_STRING, IDC_SEARCH_ALTERNATES, CTSTRING(SEARCH_FOR_ALTERNATES));
				singleMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)previewMenu, CTSTRING(PREVIEW_MENU));	
				singleMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)segmentsMenu, CTSTRING(MAX_SEGMENTS_NUMBER));
				singleMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)priorityMenu, CTSTRING(SET_PRIORITY));
				singleMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)browseMenu, CTSTRING(GET_FILE_LIST));
				singleMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)pmMenu, CTSTRING(SEND_PRIVATE_MESSAGE));
				singleMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)readdMenu, CTSTRING(READD_SOURCE));
				singleMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)copyMenu, CTSTRING(COPY));
				singleMenu.AppendMenu(MF_SEPARATOR);
				singleMenu.AppendMenu(MF_STRING, IDC_MOVE, CTSTRING(MOVE));
				singleMenu.AppendMenu(MF_SEPARATOR);
				singleMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)removeMenu, CTSTRING(REMOVE_SOURCE));
				singleMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)removeAllMenu, CTSTRING(REMOVE_FROM_ALL));
				singleMenu.AppendMenu(MF_STRING, IDC_REMOVE_OFFLINE, CTSTRING(REMOVE_OFFLINE));
				singleMenu.AppendMenu(MF_SEPARATOR);
				singleMenu.AppendMenu(MF_STRING, IDC_RECHECK, CTSTRING(RECHECK_FILE));
				singleMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));

				const QueueItemInfo* ii = ctrlQueue.getItemData(ctrlQueue.GetNextItem(-1, LVNI_SELECTED));
				QueueItem* qi = QueueManager::getInstance()->fileQueue.find(ii->getTarget());
				if(!qi) return 0;

				segmentsMenu.CheckMenuItem(qi->getMaxSegments(), MF_BYPOSITION | MF_CHECKED);

				if((ii->isSet(QueueItem::FLAG_USER_LIST)) == false) {
					string ext = Util::getFileExt(ii->getTarget());
					if(ext.size()>1) ext = ext.substr(1);
					PreviewAppsSize = WinUtil::SetupPreviewMenu(previewMenu, ext);
					if(previewMenu.GetMenuItemCount() > 1) {
						singleMenu.EnableMenuItem((UINT)(HMENU)previewMenu, MFS_ENABLED);
					} else {
						singleMenu.EnableMenuItem((UINT)(HMENU)previewMenu, MFS_DISABLED);
					}
				} else {
					singleMenu.EnableMenuItem((UINT)(HMENU)segmentsMenu, MFS_DISABLED);
				}

				menuItems = 0;
				int pmItems = 0;
				if(ii) {
					sources = QueueManager::getInstance()->getSources(ii->getQueueItem());
					for(QueueItem::SourceConstIter i = sources.begin(); i != sources.end(); ++i) {
						tstring nick = WinUtil::escapeMenu(WinUtil::getNicks(i->getUser()));
						// add hub hint to menu
						if(!i->getUser().hint.empty())
							nick += _T(" (") + Text::toT(i->getUser().hint) + _T(")");

						mi.fMask = MIIM_ID | MIIM_TYPE | MIIM_DATA;
						mi.fType = MFT_STRING;
						mi.dwTypeData = (LPTSTR)nick.c_str();
						mi.dwItemData = (ULONG_PTR)&(*i);
						mi.wID = IDC_BROWSELIST + menuItems;
						browseMenu.InsertMenuItem(menuItems, TRUE, &mi);
						mi.wID = IDC_REMOVE_SOURCE + 1 + menuItems; // "All" is before sources
						removeMenu.InsertMenuItem(menuItems + 2, TRUE, &mi); // "All" and separator come first
						mi.wID = IDC_REMOVE_SOURCES + menuItems;
						removeAllMenu.InsertMenuItem(menuItems, TRUE, &mi);
						if(i->getUser().user->isOnline()) {
							mi.wID = IDC_PM + menuItems;
							pmMenu.InsertMenuItem(menuItems, TRUE, &mi);
							pmItems++;
						}
						menuItems++;
					}
					readdItems = 0;
					
					badSources = QueueManager::getInstance()->getBadSources(ii->getQueueItem());
					for(QueueItem::SourceConstIter i = badSources.begin(); i != badSources.end(); ++i) {
						tstring nick = WinUtil::getNicks(i->getUser());
						if(i->isSet(QueueItem::Source::FLAG_FILE_NOT_AVAILABLE)) {
							nick += _T(" (") + TSTRING(FILE_NOT_AVAILABLE) + _T(")");
						} else if(i->isSet(QueueItem::Source::FLAG_PASSIVE)) {
							nick += _T(" (") + TSTRING(PASSIVE_USER) + _T(")");
						} else if(i->isSet(QueueItem::Source::FLAG_BAD_TREE)) {
							nick += _T(" (") + TSTRING(INVALID_TREE) + _T(")");
						} else if(i->isSet(QueueItem::Source::FLAG_NO_NEED_PARTS)) {
							nick += _T(" (") + TSTRING(NO_NEEDED_PART) + _T(")");
						} else if(i->isSet(QueueItem::Source::FLAG_NO_TTHF)) {
							nick += _T(" (") + TSTRING(SOURCE_TOO_OLD) + _T(")");
						} else if(i->isSet(QueueItem::Source::FLAG_SLOW_SOURCE)) {
							nick += _T(" (") + TSTRING(SLOW_USER) + _T(")");
						} else if(i->isSet(QueueItem::Source::FLAG_UNTRUSTED)) {
							nick += _T(" (") + TSTRING(CERTIFICATE_NOT_TRUSTED) + _T(")");
						}

						// add hub hint to menu
						if(!i->getUser().hint.empty())
							nick += _T(" (") + Text::toT(i->getUser().hint) + _T(")");

						mi.fMask = MIIM_ID | MIIM_TYPE | MIIM_DATA;
						mi.fType = MFT_STRING;
						mi.dwTypeData = (LPTSTR)nick.c_str();
						mi.dwItemData = (ULONG_PTR)&(*i);
						mi.wID = IDC_READD + 1 + readdItems;  // "All" is before sources
						readdMenu.InsertMenuItem(readdItems + 2, TRUE, &mi);  // "All" and separator come first
						readdItems++;
					}
				}

				if(menuItems == 0) {
					singleMenu.EnableMenuItem((UINT_PTR)(HMENU)browseMenu, MFS_DISABLED);
					singleMenu.EnableMenuItem((UINT_PTR)(HMENU)removeMenu, MFS_DISABLED);
					singleMenu.EnableMenuItem((UINT_PTR)(HMENU)removeAllMenu, MFS_DISABLED);
				}
				else {
					singleMenu.EnableMenuItem((UINT_PTR)(HMENU)browseMenu, MFS_ENABLED);
					singleMenu.EnableMenuItem((UINT_PTR)(HMENU)removeMenu, MFS_ENABLED);
					singleMenu.EnableMenuItem((UINT_PTR)(HMENU)removeAllMenu, MFS_ENABLED);
				}

				if(pmItems == 0) {
					singleMenu.EnableMenuItem((UINT_PTR)(HMENU)pmMenu, MFS_DISABLED);
				}
				else {
					singleMenu.EnableMenuItem((UINT_PTR)(HMENU)pmMenu, MFS_ENABLED);
				}

				if(readdItems == 0) {
					singleMenu.EnableMenuItem((UINT_PTR)(HMENU)readdMenu, MFS_DISABLED);
				}
				else {
					singleMenu.EnableMenuItem((UINT_PTR)(HMENU)readdMenu, MFS_ENABLED);
 				}

				priorityMenu.CheckMenuItem(ii->getPriority() + 1, MF_BYPOSITION | MF_CHECKED);
				if(ii->getAutoPriority())
					priorityMenu.CheckMenuItem(7, MF_BYPOSITION | MF_CHECKED);

				browseMenu.InsertSeparatorFirst(TSTRING(GET_FILE_LIST));
				removeMenu.InsertSeparatorFirst(TSTRING(REMOVE_SOURCE));
				removeAllMenu.InsertSeparatorFirst(TSTRING(REMOVE_FROM_ALL));
				pmMenu.InsertSeparatorFirst(TSTRING(SEND_PRIVATE_MESSAGE));
				readdMenu.InsertSeparatorFirst(TSTRING(READD_SOURCE));

				singleMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);

				browseMenu.RemoveFirstItem();
				removeMenu.RemoveFirstItem();
				removeAllMenu.RemoveFirstItem();
				pmMenu.RemoveFirstItem();
				readdMenu.RemoveFirstItem();
			} else {
				OMenu multiMenu;
				multiMenu.CreatePopupMenu();
				multiMenu.InsertSeparatorFirst(TSTRING(FILES));
				multiMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)segmentsMenu, CTSTRING(MAX_SEGMENTS_NUMBER));
				multiMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)priorityMenu, CTSTRING(SET_PRIORITY));
				multiMenu.AppendMenu(MF_STRING, IDC_MOVE, CTSTRING(MOVE));
				multiMenu.AppendMenu(MF_SEPARATOR);
				multiMenu.AppendMenu(MF_STRING, IDC_REMOVE_OFFLINE, CTSTRING(REMOVE_OFFLINE));
				multiMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));
				multiMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
			}
		
			return TRUE; 
		}
	} else if (reinterpret_cast<HWND>(wParam) == ctrlDirs && ctrlDirs.GetSelectedItem() != NULL) { 
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlDirs, pt);
		} else {
			// Strange, windows doesn't change the selection on right-click... (!)
			UINT a = 0;
			ctrlDirs.ScreenToClient(&pt);
			HTREEITEM ht = ctrlDirs.HitTest(pt, &a);
			if(ht != NULL && ht != ctrlDirs.GetSelectedItem())
				ctrlDirs.SelectItem(ht);
			ctrlDirs.ClientToScreen(&pt);
		}
		usingDirMenu = true;
		
		OMenu dirMenu;
		dirMenu.CreatePopupMenu();	
		dirMenu.InsertSeparatorFirst(TSTRING(FOLDER));
		dirMenu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)priorityMenu, CTSTRING(SET_PRIORITY));
		dirMenu.AppendMenu(MF_STRING, IDC_MOVE, CTSTRING(MOVE));
		dirMenu.AppendMenu(MF_SEPARATOR);
		dirMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));
		dirMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
	
		return TRUE;
	}

	bHandled = FALSE;
	return FALSE; 
}

LRESULT QueueFrame::onRecheck(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlQueue.GetSelectedCount() == 1) {
		int i = ctrlQueue.GetNextItem(-1, LVNI_SELECTED);
		const QueueItemInfo* ii = ctrlQueue.getItemData(i);
		QueueManager::getInstance()->recheck(ii->getTarget());
	}	
	return 0;
}

LRESULT QueueFrame::onSearchAlternates(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlQueue.GetSelectedCount() == 1) {
		int i = ctrlQueue.GetNextItem(-1, LVNI_SELECTED);
		const QueueItemInfo* ii = ctrlQueue.getItemData(i);
		WinUtil::searchHash(ii->getTTH());
	}	
	return 0;
}

LRESULT QueueFrame::onCopyMagnet(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlQueue.GetSelectedCount() == 1) {
		int i = ctrlQueue.GetNextItem(-1, LVNI_SELECTED);
		const QueueItemInfo* ii = ctrlQueue.getItemData(i);
		WinUtil::copyMagnet(ii->getTTH(), Util::getFileName(ii->getTarget()), ii->getSize());
	}
	return 0;
}

LRESULT QueueFrame::onBrowseList(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	
	if(ctrlQueue.GetSelectedCount() == 1) {
		CMenuItemInfo mi;
		mi.fMask = MIIM_DATA;
		
		browseMenu.GetMenuItemInfo(wID, FALSE, &mi);
		OMenuItem* omi = (OMenuItem*)mi.dwItemData;
		QueueItem::Source* s = (QueueItem::Source*)omi->data;
		try {
			QueueManager::getInstance()->addList(s->getUser(), QueueItem::FLAG_CLIENT_VIEW);
		} catch(const Exception&) {
		}
	}
	return 0;
}

LRESULT QueueFrame::onReadd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	
	if(ctrlQueue.GetSelectedCount() == 1) {
		int i = ctrlQueue.GetNextItem(-1, LVNI_SELECTED);
		const QueueItemInfo* ii = ctrlQueue.getItemData(i);

		CMenuItemInfo mi;
		mi.fMask = MIIM_DATA;
		
		readdMenu.GetMenuItemInfo(wID, FALSE, &mi);
		if(wID == IDC_READD) {
			// re-add all sources
			QueueItem::SourceList badSources = QueueManager::getInstance()->getBadSources(ii->getQueueItem());
			for(QueueItem::SourceConstIter s = badSources.begin(); s != badSources.end(); s++) {
				QueueManager::getInstance()->readd(ii->getTarget(), s->getUser());
			}
		} else {
			OMenuItem* omi = (OMenuItem*)mi.dwItemData;
			QueueItem::Source* s = (QueueItem::Source*)omi->data;
			try {
				QueueManager::getInstance()->readd(ii->getTarget(), s->getUser());
			} catch(const Exception& e) {
				ctrlStatus.SetText(1, Text::toT(e.getError()).c_str());
			}
		}
	}
	return 0;
}

LRESULT QueueFrame::onRemoveSource(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	
	if(ctrlQueue.GetSelectedCount() == 1) {
		int i = ctrlQueue.GetNextItem(-1, LVNI_SELECTED);
		const QueueItemInfo* ii = ctrlQueue.getItemData(i);
		if(wID == IDC_REMOVE_SOURCE) {
			QueueItem::SourceList sources = QueueManager::getInstance()->getSources(ii->getQueueItem());
			for(QueueItem::SourceConstIter si = sources.begin(); si != sources.end(); si++) {
				QueueManager::getInstance()->removeSource(ii->getTarget(), si->getUser(), QueueItem::Source::FLAG_REMOVED);
			}
		} else {
			CMenuItemInfo mi;
			mi.fMask = MIIM_DATA;
		
			removeMenu.GetMenuItemInfo(wID, FALSE, &mi);
			OMenuItem* omi = (OMenuItem*)mi.dwItemData;
			QueueItem::Source* s = (QueueItem::Source*)omi->data;
			QueueManager::getInstance()->removeSource(ii->getTarget(), s->getUser(), QueueItem::Source::FLAG_REMOVED);
		}
	}
	return 0;
}

LRESULT QueueFrame::onRemoveSources(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CMenuItemInfo mi;
	mi.fMask = MIIM_DATA;
	removeAllMenu.GetMenuItemInfo(wID, FALSE, &mi);
	OMenuItem* omi = (OMenuItem*)mi.dwItemData;
	QueueItem::Source* s = (QueueItem::Source*)omi->data;
	QueueManager::getInstance()->removeSource(s->getUser(), QueueItem::Source::FLAG_REMOVED);
	return 0;
}

LRESULT QueueFrame::onPM(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlQueue.GetSelectedCount() == 1) {
		CMenuItemInfo mi;
		mi.fMask = MIIM_DATA;
		
		pmMenu.GetMenuItemInfo(wID, FALSE, &mi);
		OMenuItem* omi = (OMenuItem*)mi.dwItemData;
		QueueItem::Source* s = (QueueItem::Source*)omi->data;
		PrivateFrame::openWindow(s->getUser());
	}
	return 0;
}

LRESULT QueueFrame::onAutoPriority(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {	

	if(usingDirMenu) {
		setAutoPriority(ctrlDirs.GetSelectedItem(), true);
	} else {
		int i = -1;
		while( (i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1) {
			QueueManager::getInstance()->setAutoPriority(ctrlQueue.getItemData(i)->getTarget(),!ctrlQueue.getItemData(i)->getAutoPriority());
		}
	}
	return 0;
}

LRESULT QueueFrame::onSegments(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while( (i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1) {
		QueueItemInfo* ii = ctrlQueue.getItemData(i);
		QueueItem* qi = QueueManager::getInstance()->fileQueue.find(ii->getTarget());

		qi->setMaxSegments((uint8_t)(wID - 109));

		ctrlQueue.updateItem(ctrlQueue.findItem(ii), COLUMN_SEGMENTS);
	}

	return 0;
}

LRESULT QueueFrame::onPriority(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	QueueItem::Priority p;

	switch(wID) {
		case IDC_PRIORITY_PAUSED: p = QueueItem::PAUSED; break;
		case IDC_PRIORITY_LOWEST: p = QueueItem::LOWEST; break;
		case IDC_PRIORITY_LOW: p = QueueItem::LOW; break;
		case IDC_PRIORITY_NORMAL: p = QueueItem::NORMAL; break;
		case IDC_PRIORITY_HIGH: p = QueueItem::HIGH; break;
		case IDC_PRIORITY_HIGHEST: p = QueueItem::HIGHEST; break;
		default: p = QueueItem::DEFAULT; break;
	}

	if(usingDirMenu) {
		setPriority(ctrlDirs.GetSelectedItem(), p);
	} else {
		int i = -1;
		while( (i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1) {
			QueueManager::getInstance()->setAutoPriority(ctrlQueue.getItemData(i)->getTarget(), false);
			QueueManager::getInstance()->setPriority(ctrlQueue.getItemData(i)->getTarget(), p);
		}
	}

	return 0;
}

void QueueFrame::removeDir(HTREEITEM ht) {
	if(ht == NULL)
		return;
	HTREEITEM child = ctrlDirs.GetChildItem(ht);
	while(child) {
		removeDir(child);
		child = ctrlDirs.GetNextSiblingItem(child);
	}
	const string& name = getDir(ht);
	DirectoryPairC dp = directories.equal_range(name);
	for(DirectoryIterC i = dp.first; i != dp.second; ++i) {
		QueueManager::getInstance()->remove(i->second->getTarget());
	}
}

/*
 * @param inc True = increase, False = decrease
 */
void QueueFrame::changePriority(bool inc){
	int i = -1;
	while( (i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1){
		QueueItem::Priority p = ctrlQueue.getItemData(i)->getPriority();

		if ((inc && p == QueueItem::HIGHEST) || (!inc && p == QueueItem::PAUSED)){
			// Trying to go higher than HIGHEST or lower than PAUSED
			// so do nothing
			continue;
		}

		switch(p){
			case QueueItem::HIGHEST: p = QueueItem::HIGH; break;
			case QueueItem::HIGH:    p = inc ? QueueItem::HIGHEST : QueueItem::NORMAL; break;
			case QueueItem::NORMAL:  p = inc ? QueueItem::HIGH    : QueueItem::LOW; break;
			case QueueItem::LOW:     p = inc ? QueueItem::NORMAL  : QueueItem::LOWEST; break;
			case QueueItem::LOWEST:  p = inc ? QueueItem::LOW     : QueueItem::PAUSED; break;
			case QueueItem::PAUSED:  p = QueueItem::LOWEST; break;
		}
		QueueManager::getInstance()->setAutoPriority(ctrlQueue.getItemData(i)->getTarget(), false);
		QueueManager::getInstance()->setPriority(ctrlQueue.getItemData(i)->getTarget(), p);
	}
}

void QueueFrame::setPriority(HTREEITEM ht, const QueueItem::Priority& p) {
	if(ht == NULL)
		return;
	HTREEITEM child = ctrlDirs.GetChildItem(ht);
	while(child) {
		setPriority(child, p);
		child = ctrlDirs.GetNextSiblingItem(child);
	}
	const string& name = getDir(ht);
	DirectoryPairC dp = directories.equal_range(name);
	for(DirectoryIterC i = dp.first; i != dp.second; ++i) {
		QueueManager::getInstance()->setAutoPriority(i->second->getTarget(), false);
		QueueManager::getInstance()->setPriority(i->second->getTarget(), p);
	}
}

void QueueFrame::setAutoPriority(HTREEITEM ht, const bool& ap) {
	if(ht == NULL)
		return;
	HTREEITEM child = ctrlDirs.GetChildItem(ht);
	while(child) {
		setAutoPriority(child, ap);
		child = ctrlDirs.GetNextSiblingItem(child);
	}
	const string& name = getDir(ht);
	DirectoryPairC dp = directories.equal_range(name);
	for(DirectoryIterC i = dp.first; i != dp.second; ++i) {
		QueueManager::getInstance()->setAutoPriority(i->second->getTarget(), ap);
	}
}

void QueueFrame::updateStatus() {
	if(ctrlStatus.IsWindow()) {
		int64_t total = 0;
		int cnt = ctrlQueue.GetSelectedCount();
		if(cnt == 0) {
			cnt = ctrlQueue.GetItemCount();
			if(showTree) {
				for(int i = 0; i < cnt; ++i) {
					const QueueItemInfo* ii = ctrlQueue.getItemData(i);
					total += (ii->getSize() > 0) ? ii->getSize() : 0;
				}
			} else {
				total = queueSize;
			}
		} else {
			int i = -1;
			while( (i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1) {
				const QueueItemInfo* ii = ctrlQueue.getItemData(i);
				total += (ii->getSize() > 0) ? ii->getSize() : 0;
			}

		}

		tstring tmp1 = TSTRING(ITEMS) + _T(": ") + Util::toStringW(cnt);
		tstring tmp2 = TSTRING(SIZE) + _T(": ") + Util::formatBytesW(total);
		bool u = false;

		int w = WinUtil::getTextWidth(tmp1, ctrlStatus.m_hWnd);
		if(statusSizes[1] < w) {
			statusSizes[1] = w;
			u = true;
		}
		ctrlStatus.SetText(2, tmp1.c_str());
		w = WinUtil::getTextWidth(tmp2, ctrlStatus.m_hWnd);
		if(statusSizes[2] < w) {
			statusSizes[2] = w;
			u = true;
		}
		ctrlStatus.SetText(3, tmp2.c_str());

		if(dirty) {
			tmp1 = TSTRING(FILES) + _T(": ") + Util::toStringW(queueItems);
			tmp2 = TSTRING(SIZE) + _T(": ") + Util::formatBytesW(queueSize);

			w = WinUtil::getTextWidth(tmp2, ctrlStatus.m_hWnd);
			if(statusSizes[3] < w) {
				statusSizes[3] = w;
				u = true;
			}
			ctrlStatus.SetText(4, tmp1.c_str());

			w = WinUtil::getTextWidth(tmp2, ctrlStatus.m_hWnd);
			if(statusSizes[4] < w) {
				statusSizes[4] = w;
				u = true;
			}
			ctrlStatus.SetText(5, tmp2.c_str());

			dirty = false;
		}

		if(u)
			UpdateLayout(TRUE);
	}
}

void QueueFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);

	if(ctrlStatus.IsWindow()) {
		CRect sr;
		int w[6];
		ctrlStatus.GetClientRect(sr);
		w[5] = sr.right - 16;
#define setw(x) w[x] = max(w[x+1] - statusSizes[x], 0)
		setw(4); setw(3); setw(2); setw(1);

		w[0] = 16;

		ctrlStatus.SetParts(6, w);

		ctrlStatus.GetRect(0, sr);
		ctrlShowTree.MoveWindow(sr);
	}

	if(showTree) {
		if(GetSinglePaneMode() != SPLIT_PANE_NONE) {
			SetSinglePaneMode(SPLIT_PANE_NONE);
			updateQueue();
		}
	} else {
		if(GetSinglePaneMode() != SPLIT_PANE_RIGHT) {
			SetSinglePaneMode(SPLIT_PANE_RIGHT);
			updateQueue();
		}
	}
	
	CRect rc = rect;
	SetSplitterRect(rc);
}

LRESULT QueueFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
		QueueManager::getInstance()->removeListener(this);
		SettingsManager::getInstance()->removeListener(this);
		closed = true;
		WinUtil::setButtonPressed(IDC_QUEUE, false);
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		HTREEITEM ht = ctrlDirs.GetRootItem();
		while(ht != NULL) {
			clearTree(ht);
			ht = ctrlDirs.GetNextSiblingItem(ht);
		}

		SettingsManager::getInstance()->set(SettingsManager::QUEUEFRAME_SHOW_TREE, ctrlShowTree.GetCheck() == BST_CHECKED);
		for(DirectoryIter i = directories.begin(); i != directories.end(); ++i) {
				delete i->second;
			}
		directories.clear();
		ctrlQueue.DeleteAllItems();

		ctrlQueue.saveHeaderOrder(SettingsManager::QUEUEFRAME_ORDER, 
			SettingsManager::QUEUEFRAME_WIDTHS, SettingsManager::QUEUEFRAME_VISIBLE);

		bHandled = FALSE;
		return 0;
	}
}

LRESULT QueueFrame::onItemChanged(int /*idCtrl*/, LPNMHDR /* pnmh */, BOOL& /*bHandled*/) {
	updateQueue();
	return 0;
}

void QueueFrame::onTab() {
	if(showTree) {
		HWND focus = ::GetFocus();
		if(focus == ctrlDirs.m_hWnd) {
			ctrlQueue.SetFocus();
		} else if(focus == ctrlQueue.m_hWnd) {
			ctrlDirs.SetFocus();
		}
	}
}

void QueueFrame::updateQueue() {
	ctrlQueue.DeleteAllItems();
	pair<DirectoryIter, DirectoryIter> i;
	if(showTree) {
		i = directories.equal_range(getSelectedDir());
	} else {
		i.first = directories.begin();
		i.second = directories.end();
	}

	ctrlQueue.SetRedraw(FALSE);
	for(DirectoryIter j = i.first; j != i.second; ++j) {
		QueueItemInfo* ii = j->second;
		ctrlQueue.insertItem(ctrlQueue.GetItemCount(), ii, WinUtil::getIconIndex(Text::toT(ii->getTarget())));
	}
	ctrlQueue.resort();
	ctrlQueue.SetRedraw(TRUE);
	curDir = getSelectedDir();
	updateStatus();
}

void QueueFrame::clearTree(HTREEITEM item) {
	HTREEITEM next = ctrlDirs.GetChildItem(item);
	while(next != NULL) {
		clearTree(next);
		next = ctrlDirs.GetNextSiblingItem(next);
	}
	delete (string*)ctrlDirs.GetItemData(item);
}

// Put it here to avoid a copy for each recursion...
static TCHAR tmpBuf[1024];
void QueueFrame::moveNode(HTREEITEM item, HTREEITEM parent) {
	TVINSERTSTRUCT tvis;
	memzero(&tvis, sizeof(tvis));
	tvis.itemex.hItem = item;
	tvis.itemex.mask = TVIF_CHILDREN | TVIF_HANDLE | TVIF_IMAGE | TVIF_INTEGRAL | TVIF_PARAM |
		TVIF_SELECTEDIMAGE | TVIF_STATE | TVIF_TEXT;
	tvis.itemex.pszText = tmpBuf;
	tvis.itemex.cchTextMax = 1024;
	ctrlDirs.GetItem((TVITEM*)&tvis.itemex);
	tvis.hInsertAfter =	TVI_SORT;
	tvis.hParent = parent;
	HTREEITEM ht = ctrlDirs.InsertItem(&tvis);
	HTREEITEM next = ctrlDirs.GetChildItem(item);
	while(next != NULL) {
		moveNode(next, ht);
		next = ctrlDirs.GetChildItem(item);
	}
	ctrlDirs.DeleteItem(item);
}

LRESULT QueueFrame::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	NMLVCUSTOMDRAW* cd = (NMLVCUSTOMDRAW*)pnmh;

	switch(cd->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT:
		{
			if(!(((QueueItemInfo*)cd->nmcd.lItemlParam)->getQueueItem()->getBadSources().empty())) {
				cd->clrText = SETTING(ERROR_COLOR);
				return CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
			}		
		}
		return CDRF_NOTIFYSUBITEMDRAW;

	case CDDS_SUBITEM | CDDS_ITEMPREPAINT: {
		if(ctrlQueue.findColumn(cd->iSubItem) == COLUMN_PROGRESS) {
			if(!BOOLSETTING(SHOW_PROGRESS_BARS)) {
				bHandled = FALSE;
				return 0;
			}			
			QueueItemInfo *qii = (QueueItemInfo*)cd->nmcd.lItemlParam;
			if(qii->getSize() == -1) return CDRF_DODEFAULT;

			// draw something nice...
			CRect rc;
			ctrlQueue.GetSubItemRect((int)cd->nmcd.dwItemSpec, cd->iSubItem, LVIR_BOUNDS, rc);
			CBarShader statusBar(rc.Height(), rc.Width(), SETTING(PROGRESS_BACK_COLOR), qii->getSize());

			vector<Segment> v;

			// running chunks
			v = QueueManager::getInstance()->getChunksVisualisation(qii->getQueueItem(), 0);
			for(vector<Segment>::const_iterator i = v.begin(); i < v.end(); ++i) {
				statusBar.FillRange((*i).getStart(), (*i).getEnd(), SETTING(COLOR_RUNNING));
			}

			// downloaded bytes
			v = QueueManager::getInstance()->getChunksVisualisation(qii->getQueueItem(), 1);
			for(vector<Segment>::const_iterator i = v.begin(); i < v.end(); ++i) {
				statusBar.FillRange((*i).getStart(), (*i).getEnd(), SETTING(COLOR_DOWNLOADED));
			}

			// done chunks
			v = QueueManager::getInstance()->getChunksVisualisation(qii->getQueueItem(), 2);
			for(vector<Segment>::const_iterator i = v.begin(); i < v.end(); ++i) {
				statusBar.FillRange((*i).getStart(), (*i).getEnd(), SETTING(COLOR_DONE));
			}

			CDC cdc;
			cdc.CreateCompatibleDC(cd->nmcd.hdc);
			HBITMAP pOldBmp = cdc.SelectBitmap(CreateCompatibleBitmap(cd->nmcd.hdc,  rc.Width(),  rc.Height()));

			statusBar.Draw(cdc, 0, 0, SETTING(PROGRESS_3DDEPTH));
			BitBlt(cd->nmcd.hdc, rc.left, rc.top, rc.Width(), rc.Height(), cdc.m_hDC, 0, 0, SRCCOPY);
			DeleteObject(cdc.SelectBitmap(pOldBmp));

			return CDRF_SKIPDEFAULT;
		}
	}
	default:
		return CDRF_DODEFAULT;
	}
}			

LRESULT QueueFrame::onCopy(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	QueueItemInfo *ii = (QueueItemInfo*)ctrlQueue.GetItemData(ctrlQueue.GetNextItem(-1, LVNI_SELECTED));

	if(ii != NULL) {
		WinUtil::setClipboard(ii->getText(wID - IDC_COPY));
	}
	return 0;
}

LRESULT QueueFrame::onPreviewCommand(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {	
	if(ctrlQueue.GetSelectedCount() == 1) {
		const QueueItemInfo* i = ctrlQueue.getItemData(ctrlQueue.GetNextItem(-1, LVNI_SELECTED));
		QueueItem* qi = QueueManager::getInstance()->fileQueue.find(i->getTarget());
		if(qi)
			WinUtil::RunPreviewCommand(wID - IDC_PREVIEW_APP, qi->getTempTarget());
	}
	return 0;
}

LRESULT QueueFrame::onRemoveOffline(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while( (i = ctrlQueue.GetNextItem(i, LVNI_SELECTED)) != -1) {
		const QueueItemInfo* ii = ctrlQueue.getItemData(i);

		QueueItem::SourceList sources = QueueManager::getInstance()->getSources(ii->getQueueItem());
		for(QueueItem::SourceConstIter i =	sources.begin(); i != sources.end(); i++) {
			if(!i->getUser().user->isOnline()) {
				QueueManager::getInstance()->removeSource(ii->getTarget(), i->getUser().user, QueueItem::Source::FLAG_REMOVED);
			}
		}
	}
	return 0;
}

void QueueFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw() {
	bool refresh = false;
	if(ctrlQueue.GetBkColor() != WinUtil::bgColor) {
		ctrlQueue.SetBkColor(WinUtil::bgColor);
		ctrlQueue.SetTextBkColor(WinUtil::bgColor);
		ctrlQueue.setFlickerFree(WinUtil::bgBrush);
		ctrlDirs.SetBkColor(WinUtil::bgColor);
		refresh = true;
	}
	if(ctrlQueue.GetTextColor() != WinUtil::textColor) {
		ctrlQueue.SetTextColor(WinUtil::textColor);
		ctrlDirs.SetTextColor(WinUtil::textColor);
		refresh = true;
	}
	if(refresh == true) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}

void QueueFrame::onRechecked(const string& target, const string& message) {
	string buf;
	buf.resize(STRING(INTEGRITY_CHECK).length() + message.length() + target.length() + 16);
	sprintf(&buf[0], CSTRING(INTEGRITY_CHECK), message.c_str(), target.c_str());
		
	speak(UPDATE_STATUS, new StringTask(&buf[0]));
}

void QueueFrame::on(QueueManagerListener::RecheckStarted, const string& target) throw() {
	onRechecked(target, STRING(STARTED));
}

void QueueFrame::on(QueueManagerListener::RecheckNoFile, const string& target) throw() {
	onRechecked(target, STRING(UNFINISHED_FILE_NOT_FOUND));
}

void QueueFrame::on(QueueManagerListener::RecheckFileTooSmall, const string& target) throw() {
	onRechecked(target, STRING(UNFINISHED_FILE_TOO_SMALL));
}

void QueueFrame::on(QueueManagerListener::RecheckDownloadsRunning, const string& target) throw() {
	onRechecked(target, STRING(DOWNLOADS_RUNNING));
}

void QueueFrame::on(QueueManagerListener::RecheckNoTree, const string& target) throw() {
	onRechecked(target, STRING(NO_FULL_TREE));
}

void QueueFrame::on(QueueManagerListener::RecheckAlreadyFinished, const string& target) throw() {
	onRechecked(target, STRING(FILE_ALREADY_FINISHED));
}

void QueueFrame::on(QueueManagerListener::RecheckDone, const string& target) throw() {
	onRechecked(target, STRING(DONE));
}
	
/**
 * @file
 * $Id$
 */
