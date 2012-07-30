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

#include "stdinc.h"
#include "DCPlusPlus.h"

#include "User.h"

#include "AdcHub.h"
#include "Client.h"
#include "StringTokenizer.h"
#include "FavoriteUser.h"

#include "ClientManager.h"
#include "DetectionManager.h"
#include "UserCommand.h"
#include "ResourceManager.h"
#include "FavoriteManager.h"

namespace dcpp {

FastCriticalSection Identity::cs;

OnlineUser::OnlineUser(const UserPtr& ptr, ClientBase& client_, uint32_t sid_) : identity(ptr, sid_), client(client_), isInList(false) { 
}

bool Identity::isTcpActive(const Client* c) const {
	if(c != NULL && user == ClientManager::getInstance()->getMe()) {
		return c->isActive(); // userlist should display our real mode
	} else {
		return (!user->isSet(User::NMDC)) ?
			!getIp().empty() && supports(AdcHub::TCP4_FEATURE) :
			!user->isSet(User::PASSIVE);	
	}
}

bool Identity::isUdpActive() const {
	if(getIp().empty() || getUdpPort().empty())
		return false;
	return (!user->isSet(User::NMDC)) ? supports(AdcHub::UDP4_FEATURE) : !user->isSet(User::PASSIVE);
}

void Identity::getParams(StringMap& sm, const string& prefix, bool compatibility, bool dht) const {
	{
		FastLock l(cs);
		for(InfIter i = info.begin(); i != info.end(); ++i) {
			sm[prefix + string((char*)(&i->first), 2)] = i->second;
		}
	}
	if(!dht && user) {
		sm[prefix + "NI"] = getNick();
		sm[prefix + "SID"] = getSIDString();
		sm[prefix + "CID"] = user->getCID().toBase32();
		sm[prefix + "TAG"] = getTag();
		sm[prefix + "CO"] = getConnection();
		sm[prefix + "SSshort"] = Util::formatBytes(get("SS"));

		if(compatibility) {
			if(prefix == "my") {
				sm["mynick"] = getNick();
				sm["mycid"] = user->getCID().toBase32();
			} else {
				sm["nick"] = getNick();
				sm["cid"] = user->getCID().toBase32();
				sm["ip"] = get("I4");
				sm["tag"] = getTag();
				sm["description"] = get("DE");
				sm["email"] = get("EM");
				sm["share"] = get("SS");
				sm["shareshort"] = Util::formatBytes(get("SS"));
				sm["realshareformat"] = Util::formatBytes(get("RS"));
			}
		}
	}
}

bool Identity::isClientType(ClientType ct) const {
	int type = Util::toInt(get("CT"));
	return (type & ct) == ct;
}

string Identity::getTag() const {
	if(!get("TA").empty())
		return get("TA");
	if(get("VE").empty() || get("HN").empty() || get("HR").empty() || get("HO").empty() || get("SL").empty())
		return Util::emptyString;
	return "<" + get("VE") + ",M:" + string(isTcpActive() ? "A" : "P") + ",H:" + get("HN") + "/" +
		get("HR") + "/" + get("HO") + ",S:" + get("SL") + ">";
}

string Identity::get(const char* name) const {
	FastLock l(cs);
	InfIter i = info.find(*(short*)name);
	return i == info.end() ? Util::emptyString : i->second;
}

bool Identity::isSet(const char* name) const {
	FastLock l(cs);
	InfIter i = info.find(*(short*)name);
	return i != info.end();
}


void Identity::set(const char* name, const string& val) {
	FastLock l(cs);
	if(val.empty())
		info.erase(*(short*)name);
	else
		info[*(short*)name] = val;
}

bool Identity::supports(const string& name) const {
	string su = get("SU");
	StringTokenizer<string> st(su, ',');
	for(StringIter i = st.getTokens().begin(); i != st.getTokens().end(); ++i) {
		if(*i == name)
			return true;
	}
	return false;
}

void FavoriteUser::update(const OnlineUser& info) { 
	setNick(info.getIdentity().getNick()); 
	setUrl(info.getClient().getHubUrl()); 
}

string Identity::setCheat(const ClientBase& c, const string& aCheatDescription, bool aBadClient) {
	if(!c.isOp() || isOp()) return Util::emptyString;

	if ((!SETTING(FAKERFILE).empty()) && (!BOOLSETTING(SOUNDS_DISABLED)))
		PlaySound(Text::toT(SETTING(FAKERFILE)).c_str(), NULL, SND_FILENAME | SND_ASYNC);
		
	StringMap ucParams;
	getParams(ucParams, "user", true);
	string cheat = Util::formatParams(aCheatDescription, ucParams, false);
	
	set("CS", cheat);

	if(aBadClient)
		set("FC", Util::toString(Util::toInt(get("FC")) | BAD_CLIENT));
	else
		set("FC", Util::toString(Util::toInt(get("FC")) & ~BAD_CLIENT));

	string report = "*** " + STRING(USER) + " " + getNick() + " - " + cheat;
	return report;
}

map<string, string> Identity::getReport() const {
	map<string, string> reportSet;

	string sid = getSIDString();

	{
		FastLock l(cs);
		for(InfIter i = info.begin(); i != info.end(); ++i) {
			string name = string((char*)(&i->first), 2);
			string value = i->second;

#define TAG(x,y) (x + (y << 8))
			
			// TODO: translate known tags and format values to something more readable
			switch(i->first) {
				case TAG('A','W'): name = "Away mode"; break;
				case TAG('B','O'): name = "Bot"; break;
				case TAG('C','L'): name = "Client name"; break;
				case TAG('C','M'): name = "Comment"; break;
				case TAG('C','O'): name = "Connection"; break;
				case TAG('C','S'): name = "Cheat description"; break;
				case TAG('C','T'): name = "Client type"; break;
				case TAG('D','E'): name = "Description"; break;
				case TAG('D','S'): name = "Download speed"; value = Util::formatBytes(value) + "/s"; break;
				case TAG('E','M'): name = "E-mail"; break;
				case TAG('F','C'): name = "Fake Check status"; break;
				case TAG('F','D'): name = "Filelist disconnects"; break;
				case TAG('G','E'): name = "Filelist generator"; break;
				case TAG('H','N'): name = "Hubs Normal"; break;
				case TAG('H','O'): name = "Hubs OP"; break;
				case TAG('H','R'): name = "Hubs Registered"; break;
				case TAG('I','4'): name = "IPv4 Address"; value += " (" + Socket::getRemoteHost(value) + ")"; break;
				case TAG('I','6'): name = "IPv6 Address"; value += " (" + Socket::getRemoteHost(value) + ")"; break;
				case TAG('I','D'): name = "Client ID"; break;
				case TAG('K','P'): name = "KeyPrint"; break;
				case TAG('L','O'): name = "NMDC Lock"; break;
				case TAG('N','I'): name = "Nick"; break;
				case TAG('O','P'): name = "Operator"; break;
				case TAG('P','K'): name = "NMDC Pk"; break;
				case TAG('R','S'): name = "Shared bytes - real"; value = Text::fromT(Util::formatExactSize(Util::toInt64(value))); break;
				case TAG('S','F'): name = "Shared files"; break;
				case TAG('S','I'): name = "Session ID"; value = sid; break;
				case TAG('S','L'): name = "Slots"; break;
				case TAG('S','S'): name = "Shared bytes - reported"; value = Text::fromT(Util::formatExactSize(Util::toInt64(value))); break;
				case TAG('S','T'): name = "NMDC Status"; value = Util::formatStatus(Util::toInt(value)); break;
				case TAG('S','U'): name = "Supports"; break;
				case TAG('T','A'): name = "Tag"; break;
				case TAG('T','O'): name = "Timeouts"; break;
				case TAG('U','4'): name = "IPv4 UDP port"; break;
				case TAG('U','6'): name = "IPv6 UDP port"; break;
				case TAG('U','S'): name = "Upload speed"; value = Util::formatBytes(value) + "/s"; break;
				case TAG('V','E'): name = "Client version"; break;
				case TAG('W','O'): name = ""; break;	// for GUI purposes
				default: name += " (unknown)";

			}

			if(!name.empty())
				reportSet.insert(make_pair(name, value));
		}
	}

	return reportSet;
}

string Identity::updateClientType(const OnlineUser& ou) {
	uint64_t tick = GET_TICK();

	StringMap params;
	getDetectionParams(params); // get identity fields and escape them, then get the rest and leave as-is
	const DetectionManager::DetectionItems& profiles = DetectionManager::getInstance()->getProfiles(params);
   
	for(DetectionManager::DetectionItems::const_iterator i = profiles.begin(); i != profiles.end(); ++i) {
		const DetectionEntry& entry = *i;
		if(!entry.isEnabled)
			continue;
		DetectionEntry::INFMap INFList;
		if(!entry.defaultMap.empty()) {
			// fields to check for both, adc and nmdc
			INFList = entry.defaultMap;
		} 

		if(getUser()->isSet(User::NMDC) && !entry.nmdcMap.empty()) {
			INFList.insert(INFList.end(), entry.nmdcMap.begin(), entry.nmdcMap.end());
		} else if(!entry.adcMap.empty()) {
			INFList.insert(INFList.end(), entry.adcMap.begin(), entry.adcMap.end());
		}

		if(INFList.empty())
			continue;

		bool _continue = false;

		DETECTION_DEBUG("\tChecking profile: " + entry.name);

		for(DetectionEntry::INFMap::const_iterator j = INFList.begin(); j != INFList.end(); ++j) {

			// TestSUR not supported anymore, so ignore it to be compatible with older profiles
			if(j->first == "TS")
				continue;

			string aPattern = Util::formatRegExp(j->second, params);
			string aField = getDetectionField(j->first);
			DETECTION_DEBUG("Pattern: " + aPattern + " Field: " + aField);
			if(!Identity::matchProfile(aField, aPattern)) {
				_continue = true;
				break;
			}
		}
		if(_continue)
			continue;

		DETECTION_DEBUG("Client found: " + entry.name + " time taken: " + Util::toString(GET_TICK()-tick) + " milliseconds");

		set("CL", entry.name);
		set("CM", entry.comment);
		
		if(entry.cheat.empty())
			set("FC", Util::toString(Util::toInt(get("FC")) & ~BAD_CLIENT));
		else
			set("FC", Util::toString(Util::toInt(get("FC")) | BAD_CLIENT));

		if(entry.checkMismatch && getUser()->isSet(User::NMDC) &&  (params["VE"] != params["PKVE"])) { 
			set("CL", entry.name + " Version mis-match");
			return setCheat(ou.getClient(), entry.cheat + " Version mis-match", true);
		}

		string report = Util::emptyString;
		if(!entry.cheat.empty()) {
			report = setCheat(ou.getClient(), entry.cheat, true);
		}

		ClientManager::getInstance()->sendRawCommand(ou, entry.rawToSend);
		return report;
	}

	set("CL", "Unknown");
	set("CS", Util::emptyString);
	set("FC", Util::toString(Util::toInt(get("FC")) & ~BAD_CLIENT));
	return Util::emptyString;
}

string Identity::getDetectionField(const string& aName) const {
	if(aName.length() == 2) {
		if(aName == "TA")
			return getTag();
		else if(aName == "CO")
			return getConnection();
		else
			return get(aName.c_str());
	} else {
		if(aName == "PKVE") {
			return getPkVersion();
		}
		return Util::emptyString;
	}
}

void Identity::getDetectionParams(StringMap& p) {
	getParams(p, Util::emptyString, false);
	p["PKVE"] = getPkVersion();
	//p["VEformat"] = getVersion();
   
	if(!user->isSet(User::NMDC)) {
		string version = get("VE");
		string::size_type i = version.find(' ');
		if(i != string::npos)
			p["VEformat"] = version.substr(i+1);
		else
			p["VEformat"] = version;
	} else {
		p["VEformat"] = get("VE");
	}

	// convert all special chars to make regex happy
	for(StringMap::iterator i = p.begin(); i != p.end(); ++i) {
		// looks really bad... but do the job
		Util::replace("\\", "\\\\", i->second); // this one must be first
		Util::replace("[", "\\[", i->second);
		Util::replace("]", "\\]", i->second);
		Util::replace("^", "\\^", i->second);
		Util::replace("$", "\\$", i->second);
		Util::replace(".", "\\.", i->second);
		Util::replace("|", "\\|", i->second);
		Util::replace("?", "\\?", i->second);
		Util::replace("*", "\\*", i->second);
		Util::replace("+", "\\+", i->second);
		Util::replace("(", "\\(", i->second);
		Util::replace(")", "\\)", i->second);
		Util::replace("{", "\\{", i->second);
		Util::replace("}", "\\}", i->second);
	}
}

string Identity::getPkVersion() const {
	string pk = get("PK");

	string::const_iterator begin = pk.begin();
	string::const_iterator end = pk.end();
	boost::match_results<string::const_iterator> result;
	boost::regex reg("[0-9]+\\.[0-9]+", boost::regex_constants::icase);
	if(boost::regex_search(begin, end, result, reg, boost::match_default))
		return result.str();

	return Util::emptyString;
}

bool Identity::matchProfile(const string& aString, const string& aProfile) const {
	DETECTION_DEBUG("\t\tMatching String: " + aString + " to Profile: " + aProfile);
	
	try {
		boost::regex reg(aProfile);
		return boost::regex_search(aString.begin(), aString.end(), reg);
	} catch(...) {
	}
	
	return false;
}

string Identity::getVersion(const string& aExp, string aTag) {
	string::size_type i = aExp.find("%[version]");
	if (i == string::npos) { 
		i = aExp.find("%[version2]"); 
		return splitVersion(aExp.substr(i + 11), splitVersion(aExp.substr(0, i), aTag, 1), 0);
	}
	return splitVersion(aExp.substr(i + 10), splitVersion(aExp.substr(0, i), aTag, 1), 0);
}

string Identity::splitVersion(const string& aExp, string aTag, size_t part) {
	try {
		boost::regex reg(aExp);

		vector<string> out;
		boost::regex_split(std::back_inserter(out), aTag, reg, boost::regex_constants::match_default, 2);
		
		if(part >= out.size())
			return Util::emptyString;
		
		return out[part];
	} catch(...) {
	}
	
	return Util::emptyString;
}

int OnlineUser::compareItems(const OnlineUser* a, const OnlineUser* b, uint8_t col)  {
	if(col == COLUMN_NICK) {
		bool a_isOp = a->getIdentity().isOp(),
			b_isOp = b->getIdentity().isOp();
		if(a_isOp && !b_isOp)
			return -1;
		if(!a_isOp && b_isOp)
			return 1;
		if(BOOLSETTING(SORT_FAVUSERS_FIRST)) {
			bool a_isFav = FavoriteManager::getInstance()->isFavoriteUser(a->getIdentity().getUser()),
				b_isFav = FavoriteManager::getInstance()->isFavoriteUser(b->getIdentity().getUser());
			if(a_isFav && !b_isFav)
				return -1;
			if(!a_isFav && b_isFav)
				return 1;
		}
		// workaround for faster hub loading
		// lstrcmpiA(a->identity.getNick().c_str(), b->identity.getNick().c_str());
	}
	switch(col) {
		case COLUMN_SHARED:
		case COLUMN_EXACT_SHARED: return compare(a->identity.getBytesShared(), b->identity.getBytesShared());
		case COLUMN_SLOTS: return compare(Util::toInt(a->identity.get("SL")), Util::toInt(b->identity.get("SL")));
		case COLUMN_HUBS: return compare(Util::toInt(a->identity.get("HN"))+Util::toInt(a->identity.get("HR"))+Util::toInt(a->identity.get("HO")), Util::toInt(b->identity.get("HN"))+Util::toInt(b->identity.get("HR"))+Util::toInt(b->identity.get("HO")));
	}
	return lstrcmpi(a->getText(col).c_str(), b->getText(col).c_str());
}

tstring OnlineUser::getText(uint8_t col) const {
	switch(col) {
		case COLUMN_NICK: return Text::toT(identity.getNick());
		case COLUMN_SHARED: return Util::formatBytesW(identity.getBytesShared());
		case COLUMN_EXACT_SHARED: return Util::formatExactSize(identity.getBytesShared());
		case COLUMN_DESCRIPTION: return Text::toT(identity.getDescription());
		case COLUMN_TAG: return Text::toT(identity.getTag());
		case COLUMN_CONNECTION: return identity.get("US").empty() ? Text::toT(identity.getConnection()) : (Text::toT(Util::formatBytes(identity.get("US"))) + _T("/s"));
		case COLUMN_IP: {
			string ip = identity.getIp();
			string country = ip.empty() ? Util::emptyString : Util::getIpCountry(ip);
			if (!country.empty())
				ip = country + " (" + ip + ")";
			return Text::toT(ip);
		}
		case COLUMN_EMAIL: return Text::toT(identity.getEmail());
		case COLUMN_VERSION: return Text::toT(identity.get("CL").empty() ? identity.get("VE") : identity.get("CL"));
		case COLUMN_MODE: return identity.isTcpActive(&getClient()) ? _T("A") : _T("P");
		case COLUMN_HUBS: {
			const tstring hn = Text::toT(identity.get("HN"));
			const tstring hr = Text::toT(identity.get("HR"));
			const tstring ho = Text::toT(identity.get("HO"));
			return (hn.empty() || hr.empty() || ho.empty()) ? Util::emptyStringT : (hn + _T("/") + hr + _T("/") + ho);
		}
		case COLUMN_SLOTS: return Text::toT(identity.get("SL"));
		case COLUMN_CID: return Text::toT(identity.getUser()->getCID().toBase32());
		default: return Util::emptyStringT;
	}
}

tstring old = Util::emptyStringT;
bool OnlineUser::update(int sortCol, const tstring& oldText) {
	bool needsSort = ((identity.get("WO").empty() ? false : true) != identity.isOp());
	
	if(sortCol == -1) {
		isInList = true;
	} else {
		needsSort = needsSort || (oldText != getText(static_cast<uint8_t>(sortCol)));
	}

	return needsSort;
}

} // namespace dcpp

/**
 * @file
 * $Id$
 */
