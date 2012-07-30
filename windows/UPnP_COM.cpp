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

#include "stdafx.h"

#include "UPnP_COM.h"

#include "../client/Util.h"
#include "../client/Text.h"

#include <ole2.h>

const string UPnP_COM::name = "Standard Windows";

#ifdef HAVE_NATUPNP_H
#include <natupnp.h>
	
bool UPnP_COM::init() {
	// Lacking the __uuidof in mingw...
	CLSID upnp;
	OLECHAR upnps[] = L"{AE1E00AA-3FD5-403C-8A27-2BBDC30CD0E1}";
	CLSIDFromString(upnps, &upnp);
	IID iupnp;
	OLECHAR iupnps[] = L"{B171C812-CC76-485A-94D8-B6B3A2794E99}";
	CLSIDFromString(iupnps, &iupnp);
	pUN = 0;
	HRESULT hr = CoCreateInstance(upnp, 0, CLSCTX_INPROC_SERVER, iupnp, reinterpret_cast<LPVOID*>(&pUN));
	if(FAILED(hr))
		pUN = 0;
	return pUN;
}

bool UPnP_COM::add(const unsigned short port, const Protocol protocol, const string& description) {
	IStaticPortMappingCollection* pSPMC = getStaticPortMappingCollection();
	if(!pSPMC)
		return false;

	/// @todo use a BSTR wrapper
	BSTR protocol_ = SysAllocString(Text::toT(protocols[protocol]).c_str());
	BSTR description_ = SysAllocString(Text::toT(description).c_str());
	BSTR localIP = SysAllocString(Text::toT(Util::getLocalIp()).c_str());

	IStaticPortMapping* pSPM = 0;
	HRESULT hr = pSPMC->Add(port, protocol_, port, localIP, VARIANT_TRUE, description_, &pSPM);

	SysFreeString(protocol_);
	SysFreeString(description_);
	SysFreeString(localIP);

	bool ret = SUCCEEDED(hr);
	if(ret) {
		pSPM->Release();
		pSPMC->Release();

		lastPort = port;
		lastProtocol = protocol;
	}
	return ret;
}

bool UPnP_COM::remove(const unsigned short port, const Protocol protocol) {
	IStaticPortMappingCollection* pSPMC = getStaticPortMappingCollection();
	if(!pSPMC)
		return false;

	/// @todo use a BSTR wrapper
	BSTR protocol_ = SysAllocString(Text::toT(protocols[protocol]).c_str());

	HRESULT hr = pSPMC->Remove(port, protocol_);
	pSPMC->Release();

	SysFreeString(protocol_);

	bool ret = SUCCEEDED(hr);
	if(ret && port == lastPort && protocol == lastProtocol) {
		lastPort = 0;
	}
	return ret;
}

string UPnP_COM::getExternalIP() {
	// Get the External IP from the last added mapping
	if(!lastPort)
		return Util::emptyString;

	IStaticPortMappingCollection* pSPMC = getStaticPortMappingCollection();
	if(!pSPMC)
		return Util::emptyString;

	/// @todo use a BSTR wrapper
	BSTR protocol_ = SysAllocString(Text::toT(protocols[lastProtocol]).c_str());

	// Lets Query our mapping
	IStaticPortMapping* pSPM;
	HRESULT hr = pSPMC->get_Item(lastPort, protocol_, &pSPM);

	SysFreeString(protocol_);

	// Query failed!
	if(FAILED(hr) || !pSPM) {
		pSPMC->Release();
		return Util::emptyString;
	}

	BSTR bstrExternal = 0;
	hr = pSPM->get_ExternalIPAddress(&bstrExternal);
	if(FAILED(hr) || !bstrExternal) {
		pSPM->Release();
		pSPMC->Release();
		return Util::emptyString;
	}

	// convert the result
	string ret = Text::wideToAcp(bstrExternal);

	// no longer needed
	SysFreeString(bstrExternal);

	// no longer needed
	pSPM->Release();
	pSPMC->Release();

	return ret;
}

IStaticPortMappingCollection* UPnP_COM::getStaticPortMappingCollection() {
	if(!pUN)
		return 0;
	IStaticPortMappingCollection* ret = 0;
	HRESULT hr = 0;

	// some routers lag here
	for(int i = 0; i < 3; i++) {
		hr = pUN->get_StaticPortMappingCollection (&ret);
			
		if(SUCCEEDED(hr) && ret) break;

		Sleep(1500);
	}

	if(FAILED(hr))
		return 0;
	return ret;
}

#else

bool UPnP_COM::init() {
	return false;
}

bool UPnP_COM::add(const unsigned short port, const Protocol protocol, const string& description) {
	return false;
}

bool UPnP_COM::remove(const unsigned short port, const Protocol protocol) {
	return false;
}

string UPnP_COM::getExternalIP() {
	return Util::emptyString;
}

IStaticPortMappingCollection* UPnP_COM::getStaticPortMappingCollection() {
	return 0;
}

#endif
