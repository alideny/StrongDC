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

#ifndef DCPLUSPLUS_DCPP_SEARCHRESULT_H
#define DCPLUSPLUS_DCPP_SEARCHRESULT_H

#include "forward.h"
#include "FastAlloc.h"
#include "MerkleTree.h"
#include "AdcCommand.h"
#include "Pointer.h"

namespace dcpp {

class SearchManager;

class SearchResult : public FastAlloc<SearchResult>, public intrusive_ptr_base<SearchResult> {
public:	
	enum Types {
		TYPE_FILE,
		TYPE_DIRECTORY
	};

	SearchResult(Types aType, int64_t aSize, const string& name, const TTHValue& aTTH);

	SearchResult(const UserPtr& aUser, Types aType, uint8_t aSlots, uint8_t aFreeSlots, 
		int64_t aSize, const string& aFile, const string& aHubName, 
			const string& aHubURL, const string& ip, TTHValue aTTH, const string& aToken);

	string getFileName() const;
	string toSR(const Client& client) const;
	AdcCommand toRES(char type) const;

	const UserPtr& getUser() const { return user; }
	string getSlotString() const { return Util::toString(getFreeSlots()) + '/' + Util::toString(getSlots()); }

	const string& getFile() const { return file; }
	const string& getHubURL() const { return hubURL; }
	const string& getHubName() const { return hubName; }
	int64_t getSize() const { return size; }
	Types getType() const { return type; }
	size_t getSlots() const { return slots; }
	size_t getFreeSlots() const { return freeSlots; }
	const TTHValue& getTTH() const { return tth; }

	// for DHT use only
	void setSlots(size_t _slots) { slots = _slots; }
	
	const string& getIP() const { return IP; }
	const string& getToken() const { return token; }

private:
	friend class SearchManager;

	SearchResult();

	SearchResult(const SearchResult& rhs);

	TTHValue tth;
	
	string file;
	string hubName;
	string hubURL;
	string IP;
	string token;
	
	int64_t size;
	
	size_t slots;
	size_t freeSlots;
	
	UserPtr user;
	Types type;


};

}

#endif
