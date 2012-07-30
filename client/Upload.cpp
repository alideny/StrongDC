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

#include "Upload.h"

#include "UserConnection.h"
#include "Streams.h"

namespace dcpp {

Upload::Upload(UserConnection& conn, const string& path, const TTHValue& tth) : Transfer(conn, path, tth), stream(0), fileSize(-1), delayTime(0) { 
	conn.setUpload(this);
}

Upload::~Upload() { 
	getUserConnection().setUpload(0);
	delete stream; 
}

void Upload::getParams(const UserConnection& aSource, StringMap& params) const {
	Transfer::getParams(aSource, params);
	params["source"] = getPath();
}

} // namespace dcpp
