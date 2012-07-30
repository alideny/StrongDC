/*
 * Copyright (C) 2001-2010 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_DCPP_FORWARD_H_
#define DCPLUSPLUS_DCPP_FORWARD_H_

/** @file
 * This file contains forward declarations for the various DC++ classes
 */

#include "Pointer.h"

namespace dcpp {

class AdcCommand;

class BufferedSocket;

struct ChatMessage;

class CID;

typedef std::vector<uint16_t> PartsInfo;

class Client;

class ClientManager;

class ConnectionQueueItem;

class Download;
typedef Download* DownloadPtr;
typedef std::vector<DownloadPtr> DownloadList;

class FavoriteHubEntry;
typedef FavoriteHubEntry* FavoriteHubEntryPtr;
typedef std::vector<FavoriteHubEntryPtr> FavoriteHubEntryList;

class FavoriteUser;

class File;

class FinishedItem;
typedef FinishedItem* FinishedItemPtr;
typedef std::vector<FinishedItemPtr> FinishedItemList;

class FinishedManager;

struct HintedUser;
typedef std::vector<HintedUser> HintedUserList;

class HubEntry;
typedef std::vector<HubEntry> HubEntryList;

class Identity;

class InputStream;

class OnlineUser;
//typedef OnlineUser* OnlineUserPtr;
typedef boost::intrusive_ptr<OnlineUser> OnlineUserPtr;
typedef std::vector<OnlineUserPtr> OnlineUserList;

class QueueItem;

class RecentHubEntry;

class SearchResult;
typedef boost::intrusive_ptr<SearchResult> SearchResultPtr;
typedef std::vector<SearchResultPtr> SearchResultList;

class ServerSocket;

class Socket;
class SocketException;

class StringSearch;

class Transfer;

class UnZFilter;

class Upload;
typedef Upload* UploadPtr;
typedef std::vector<UploadPtr> UploadList;

class UploadQueueItem;

class User;
typedef boost::intrusive_ptr<User> UserPtr;
typedef std::vector<UserPtr> UserList;

class UserCommand;

class UserConnection;
typedef UserConnection* UserConnectionPtr;
typedef std::vector<UserConnectionPtr> UserConnectionList;

} // namespace dcpp

#endif /*DCPLUSPLUS_CLIENT_FORWARD_H_*/
