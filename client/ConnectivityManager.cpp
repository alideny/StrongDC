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

#include "ConnectivityManager.h"
#include "SettingsManager.h"
#include "ClientManager.h"
#include "ConnectionManager.h"
#include "SearchManager.h"
#include "LogManager.h"
#include "UPnPManager.h"

#include "../dht/dht.h"

namespace dcpp {

ConnectivityManager::ConnectivityManager() :
autoDetected(false),
running(false)
{
}

void ConnectivityManager::startSocket() {
	autoDetected = false;

	disconnect();

	// active check mustn't be there to hub-dependent setting works correctly
	//if(ClientManager::getInstance()->isActive()) {
		listen();

		// must be done after listen calls; otherwise ports won't be set
		if(SETTING(INCOMING_CONNECTIONS) == SettingsManager::INCOMING_FIREWALL_UPNP)
			UPnPManager::getInstance()->open();
	//}
}

void ConnectivityManager::detectConnection() {
	if (running)
		return;

	running = true;

	if (UPnPManager::getInstance()->getOpened()) {
		UPnPManager::getInstance()->close();
	}

	disconnect();

	log("Determining connection type...");
	try {
		listen();
	} catch(const Exception& e) {
		SettingsManager::getInstance()->set(SettingsManager::INCOMING_CONNECTIONS, SettingsManager::INCOMING_FIREWALL_PASSIVE);
		SettingsManager::getInstance()->set(SettingsManager::ALLOW_NAT_TRAVERSAL, true);
		log("Unable to open " + e.getError() + " port(s). You must set up your connection manually");
		fire(ConnectivityManagerListener::Finished());
		running = false;
		return;
	}

	autoDetected = true;

	if (!Util::isPrivateIp(Util::getLocalIp())) {
		SettingsManager::getInstance()->set(SettingsManager::INCOMING_CONNECTIONS, SettingsManager::INCOMING_DIRECT);
		log("Public IP address detected, selecting active mode with direct connection");
		fire(ConnectivityManagerListener::Finished());
		running = false;
		return;
	}

	SettingsManager::getInstance()->set(SettingsManager::INCOMING_CONNECTIONS, SettingsManager::INCOMING_FIREWALL_UPNP);
	log("Local network with possible NAT detected, trying to map the ports using UPnP...");
	
	if (!UPnPManager::getInstance()->open()) {
		running = false;
	}
}

void ConnectivityManager::setup(bool settingsChanged, int lastConnectionMode) {
	if(BOOLSETTING(AUTO_DETECT_CONNECTION)) {
		if (!autoDetected) detectConnection();
	} else {
		if(autoDetected || settingsChanged) {
			if(SETTING(INCOMING_CONNECTIONS) == SettingsManager::INCOMING_FIREWALL_UPNP || lastConnectionMode == SettingsManager::INCOMING_FIREWALL_UPNP) {
				UPnPManager::getInstance()->close();
			}
			startSocket();
		} else if(SETTING(INCOMING_CONNECTIONS) == SettingsManager::INCOMING_FIREWALL_UPNP && !UPnPManager::getInstance()->getOpened()) {
			// previous UPnP mappings had failed; try again
			UPnPManager::getInstance()->open();
		}
	}
}

void ConnectivityManager::mappingFinished(bool success) {
	if(BOOLSETTING(AUTO_DETECT_CONNECTION)) {
		if (!success) {
			disconnect();
			SettingsManager::getInstance()->set(SettingsManager::INCOMING_CONNECTIONS, SettingsManager::INCOMING_FIREWALL_PASSIVE);
			SettingsManager::getInstance()->set(SettingsManager::ALLOW_NAT_TRAVERSAL, true);
			log("Automatic setup of active mode has failed. You may want to set up your connection manually for better connectivity");
		}
		fire(ConnectivityManagerListener::Finished());
	}

	running = false;
}

void ConnectivityManager::listen() {
	try {
		ConnectionManager::getInstance()->listen();
	} catch(const Exception&) {
		throw Exception("TCP/TLS");
	}

	try {
		SearchManager::getInstance()->listen();
	} catch(const Exception&) {
		throw Exception("UDP");
	}

	try {
		dht::DHT::getInstance()->start();
	} catch(const Exception&) {
		throw Exception("DHT");
	}
}

void ConnectivityManager::disconnect() {
	SearchManager::getInstance()->disconnect();
	ConnectionManager::getInstance()->disconnect();
	dht::DHT::getInstance()->stop();
}

void ConnectivityManager::log(const string& message) {
	if(BOOLSETTING(AUTO_DETECT_CONNECTION)) {
		LogManager::getInstance()->message("Connectivity: " + message);
		fire(ConnectivityManagerListener::Message(), message);
	} else {
		LogManager::getInstance()->message(message);
	}
}

} // namespace dcpp
