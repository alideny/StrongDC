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

#if !defined(HTTP_CONNECTION_H)
#define HTTP_CONNECTION_H

#include "BufferedSocket.h"

namespace dcpp {

class HttpConnection;

class HttpConnectionListener {
public:
	virtual ~HttpConnectionListener() { }
	template<int I>	struct X { enum { TYPE = I };  };

	typedef X<0> Data;
	typedef X<1> Failed;
	typedef X<2> Complete;
	typedef X<3> Redirected;
	typedef X<4> TypeNormal;
	typedef X<5> TypeBZ2;
	typedef X<6> Retried;

	virtual void on(Data, HttpConnection*, const uint8_t*, size_t) throw() =0;
	virtual void on(Failed, HttpConnection*, const string&) throw() { }
	virtual void on(Complete, HttpConnection*, const string&, bool) throw() { }
	virtual void on(Redirected, HttpConnection*, const string&) throw() { }
	virtual void on(TypeNormal, HttpConnection*) throw() { }
	virtual void on(TypeBZ2, HttpConnection*) throw() { }
	virtual void on(Retried, HttpConnection*, const bool) throw() { }
};

class HttpConnection : BufferedSocketListener, public Speaker<HttpConnectionListener>
{
public:
	void downloadFile(const string& aUrl);
	HttpConnection() : ok(false), port(80), size(-1), moved302(false), coralizeState(CST_DEFAULT), socket(NULL) { }
	~HttpConnection() throw() {
		if(socket) {
			socket->removeListener(this); 
			BufferedSocket::putSocket(socket);
		}
	}
	
	enum CoralizeStates {CST_DEFAULT, CST_CONNECTED, CST_NOCORALIZE};
	void setCoralizeState(CoralizeStates _cor) { coralizeState = _cor; }
	
private:

	HttpConnection(const HttpConnection&);
	HttpConnection& operator=(const HttpConnection&);

	string currentUrl;
	string file;
	string server;
	bool ok;
	uint16_t port;
	int64_t size;
	bool moved302;

	CoralizeStates coralizeState;

	BufferedSocket* socket;

	// BufferedSocketListener
	void on(Connected) throw();
	void on(Line, const string&) throw();
	void on(Data, uint8_t*, size_t) throw();
	void on(ModeChange) throw();
	void on(Failed, const string&) throw();

	void onConnected(); 
	void onLine(const string& aLine);
	
};

} // namespace dcpp

#endif // !defined(HTTP_CONNECTION_H)

/**
 * @file
 * $Id$
 */
