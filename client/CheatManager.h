/**
 * This file is a part of client manager.
 * It has been divided but shouldn't be used anywhere else.
 */

void sendRawCommand(const OnlineUser& ou, const int aRawCommand) {
	string rawCommand = ou.getClient().getRawCommand(aRawCommand);
	if (!rawCommand.empty()) {
		StringMap ucParams;

		UserCommand uc = UserCommand(0, 0, 0, 0, "", rawCommand, "", "");
		userCommand(HintedUser(ou.getUser(), ou.getClient().getHubUrl()), uc, ucParams, true);
	}
}

void setListLength(const UserPtr& p, const string& listLen) {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(const_cast<CID*>(&p->getCID()));
	if(i != onlineUsers.end()) {
		i->second->getIdentity().set("LL", listLen);
	}
}

void fileListDisconnected(const UserPtr& p) {
	string report = Util::emptyString;
	Client* c = NULL;
	{
		Lock l(cs);
		OnlineIterC i = onlineUsers.find(const_cast<CID*>(&p->getCID()));
		if(i != onlineUsers.end() && i->second->getClientBase().type != ClientBase::DHT) {
			OnlineUser& ou = *i->second;
	
			int fileListDisconnects = Util::toInt(ou.getIdentity().get("FD")) + 1;
			ou.getIdentity().set("FD", Util::toString(fileListDisconnects));

			if(SETTING(ACCEPTED_DISCONNECTS) == 0)
				return;

			if(fileListDisconnects == SETTING(ACCEPTED_DISCONNECTS)) {
				c = &ou.getClient();
				report = ou.getIdentity().setCheat(ou.getClientBase(), "Disconnected file list " + Util::toString(fileListDisconnects) + " times", false);
				getInstance()->sendRawCommand(ou, SETTING(DISCONNECT_RAW));
			}
		}
	}
	if(c && !report.empty() && BOOLSETTING(DISPLAY_CHEATS_IN_MAIN_CHAT)) {
		c->cheatMessage(report);
	}
}

void connectionTimeout(const UserPtr& p) {
	string report = Util::emptyString;
	bool remove = false;
	Client* c = NULL;
	{
		Lock l(cs);
		OnlineIterC i = onlineUsers.find(const_cast<CID*>(&p->getCID()));
		if(i != onlineUsers.end() && i->second->getClientBase().type != ClientBase::DHT) {
			OnlineUser& ou = *i->second;
	
			int connectionTimeouts = Util::toInt(ou.getIdentity().get("TO")) + 1;
			ou.getIdentity().set("TO", Util::toString(connectionTimeouts));
	
			if(SETTING(ACCEPTED_TIMEOUTS) == 0)
				return;
	
			if(connectionTimeouts == SETTING(ACCEPTED_TIMEOUTS)) {
				c = &ou.getClient();
				report = ou.getIdentity().setCheat(ou.getClientBase(), "Connection timeout " + Util::toString(connectionTimeouts) + " times", false);
				remove = true;
				sendRawCommand(ou, SETTING(TIMEOUT_RAW));
			}
		}
	}
	if(remove) {
		try {
			// TODO: remove user check
		} catch(...) {
		}
	}
	if(c && !report.empty() && BOOLSETTING(DISPLAY_CHEATS_IN_MAIN_CHAT)) {
		c->cheatMessage(report);
	}
}

void checkCheating(const UserPtr& p, DirectoryListing* dl) {
	string report = Util::emptyString;
	OnlineUserPtr ou = NULL;
	{
		Lock l(cs);

		OnlineIterC i = onlineUsers.find(const_cast<CID*>(&p->getCID()));
		if(i == onlineUsers.end() || i->second->getClientBase().type == ClientBase::DHT)
			return;

		ou = i->second;

		int64_t statedSize = ou->getIdentity().getBytesShared();
		int64_t realSize = dl->getTotalSize();
	
		double multiplier = ((100+(double)SETTING(PERCENT_FAKE_SHARE_TOLERATED))/100); 
		int64_t sizeTolerated = (int64_t)(realSize*multiplier);
		string detectString = Util::emptyString;
		string inflationString = Util::emptyString;
		ou->getIdentity().set("RS", Util::toString(realSize));
		bool isFakeSharing = false;
	
		if(statedSize > sizeTolerated) {
			isFakeSharing = true;
		}

		if(isFakeSharing) {
			ou->getIdentity().set("FC", Util::toString(Util::toInt(ou->getIdentity().get("FC")) | Identity::BAD_LIST));
			detectString += STRING(CHECK_MISMATCHED_SHARE_SIZE) + " ";
			if(realSize == 0) {
				detectString += STRING(CHECK_0BYTE_SHARE);
			} else {
				double qwe = (double)((double)statedSize / (double)realSize);
				char buf[128];
				snprintf(buf, sizeof(buf), CSTRING(CHECK_INFLATED), Util::toString(qwe).c_str());
				inflationString = buf;
				detectString += inflationString;
			}
			detectString += STRING(CHECK_SHOW_REAL_SHARE);

			report = ou->getIdentity().setCheat(ou->getClientBase(), detectString, false);
			sendRawCommand(*ou.get(), SETTING(FAKESHARE_RAW));
		}
		ou->getIdentity().set("FC", Util::toString(Util::toInt(ou->getIdentity().get("FC")) | Identity::CHECKED));

		if(report.empty())
			report = ou->getIdentity().updateClientType(*ou);
		else
			ou->getIdentity().updateClientType(*ou);
	}
	ou->getClient().updated(ou);
	if(!report.empty() && BOOLSETTING(DISPLAY_CHEATS_IN_MAIN_CHAT))
		ou->getClient().cheatMessage(report);
}

void setClientStatus(const UserPtr& p, const string& aCheatString, const int aRawCommand, bool aBadClient) {
	OnlineUserPtr ou = NULL;
	string report = Util::emptyString;
	{
		Lock l(cs);
		OnlineIterC i = onlineUsers.find(const_cast<CID*>(&p->getCID()));
		if(i == onlineUsers.end() || i->second->getClientBase().type == ClientBase::DHT) 
			return;
		
		ou = i->second;
		report = ou->getIdentity().updateClientType(*ou);

		if(!aCheatString.empty()) {
			report = ou->getIdentity().setCheat(ou->getClientBase(), aCheatString, aBadClient);
		}
		if(aRawCommand != -1)
			sendRawCommand(*ou.get(), aRawCommand);
	}
	ou->getClient().updated(ou);
	if(!report.empty() && BOOLSETTING(DISPLAY_CHEATS_IN_MAIN_CHAT))
		ou->getClient().cheatMessage(report);
}

void setPkLock(const UserPtr& p, const string& aPk, const string& aLock) {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(const_cast<CID*>(&p->getCID()));
	if(i == onlineUsers.end()) return;
	
	i->second->getIdentity().set("PK", aPk);
	i->second->getIdentity().set("LO", aLock);
}

void setSupports(const UserPtr& p, const string& aSupports) {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(const_cast<CID*>(&p->getCID()));
	if(i == onlineUsers.end()) return;
	
	i->second->getIdentity().set("SU", aSupports);
}

void setGenerator(const UserPtr& p, const string& aGenerator) {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(const_cast<CID*>(&p->getCID()));
	if(i == onlineUsers.end()) return;
	i->second->getIdentity().set("GE", aGenerator);
}

void setUnknownCommand(const UserPtr& p, const string& aUnknownCommand) {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(const_cast<CID*>(&p->getCID()));
	if(i == onlineUsers.end()) return;
	i->second->getIdentity().set("UC", aUnknownCommand);
}

void reportUser(const HintedUser& user) {
	bool priv = FavoriteManager::getInstance()->isPrivate(user.hint);
	string nick; string report;

	{
		Lock l(cs);
		OnlineUser* ou = findOnlineUser(user.user->getCID(), user.hint, priv);
		if(!ou || ou->getClientBase().type == ClientBase::DHT) 
			return;

		ou->getClient().reportUser(ou->getIdentity());
	}
}
