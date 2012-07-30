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

#ifndef DCPLUSPLUS_DCPP_CRYPTO_MANAGER_H
#define DCPLUSPLUS_DCPP_CRYPTO_MANAGER_H

#include "SettingsManager.h"

#include "Exception.h"
#include "Singleton.h"
#include "FastAlloc.h"
#include "version.h"
#include "SSLSocket.h"

namespace dcpp {

STANDARD_EXCEPTION(CryptoException);

class File;
class FileException;

class CryptoManager : public Singleton<CryptoManager>
{
public:
	string makeKey(const string& aLock);
	const string& getLock() const { return lock; }
	const string& getPk() const  { return pk; }
	bool isExtended(const string& aLock) const { return strncmp(aLock.c_str(), "EXTENDEDPROTOCOL", 16) == 0; }

	void decodeBZ2(const uint8_t* is, unsigned int sz, string& os) throw(CryptoException);

	SSLSocket* getClientSocket(bool allowUntrusted) throw(SocketException);
	SSLSocket* getServerSocket(bool allowUntrusted) throw(SocketException);

	void loadCertificates() throw();
	void generateCertificate() throw(CryptoException);
	bool checkCertificate() throw();

	bool TLSOk() const throw();

#ifdef HEADER_OPENSSLV_H	
	static void __cdecl locking_function(int mode, int n, const char *file, int line);
#endif


private:

	friend class Singleton<CryptoManager>;
	
	CryptoManager();
	~CryptoManager();

	ssl::SSL_CTX clientContext;
	ssl::SSL_CTX clientVerContext;
	ssl::SSL_CTX serverContext;
	ssl::SSL_CTX serverVerContext;

#ifdef HEADER_OPENSSLV_H	
	ssl::DH dh;
#endif
	
	bool certsLoaded;

	const string lock;
	const string pk;
	
#ifdef HEADER_OPENSSLV_H
	static CriticalSection* cs;
#endif

	string keySubst(const uint8_t* aKey, size_t len, size_t n);
	bool isExtra(uint8_t b) const {
		return (b == 0 || b==5 || b==124 || b==96 || b==126 || b==36);
	}
};

} // namespace dcpp

#endif // !defined(CRYPTO_MANAGER_H)

/**
 * @file
 * $Id$
 */
