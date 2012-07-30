#include "StdAfx.h"

#include "BootstrapManager.h"
#include "DHT.h"
#include "IndexManager.h"
#include "SearchManager.h"
#include "TaskManager.h"
#include "Utils.h"

#include "../client/SettingsManager.h"
#include "../client/ShareManager.h"
#include "../client/TimerManager.h"

namespace dht
{

	TaskManager::TaskManager(void) :
		nextPublishTime(GET_TICK()), nextSearchTime(GET_TICK()), nextSelfLookup(GET_TICK() + 3*60*1000),
		nextFirewallCheck(GET_TICK() + FWCHECK_TIME), lastBootstrap(0)
	{
		TimerManager::getInstance()->addListener(this);
	}

	TaskManager::~TaskManager(void)
	{
		TimerManager::getInstance()->removeListener(this);
	}
	
	// TimerManagerListener
	void TaskManager::on(TimerManagerListener::Second, uint64_t aTick) throw()
	{	
		if(DHT::getInstance()->isConnected() && DHT::getInstance()->getNodesCount() >= K)
		{
			if(!DHT::getInstance()->isFirewalled() && IndexManager::getInstance()->getPublish() && aTick >= nextPublishTime)
			{
				// publish next file
				IndexManager::getInstance()->publishNextFile();
				nextPublishTime = aTick + PUBLISH_TIME;
			}
		}
		else
		{
			if(aTick - lastBootstrap > 15000 || (DHT::getInstance()->getNodesCount() == 0 && aTick - lastBootstrap >= 2000))
			{
				// bootstrap if we doesn't know any remote node
				BootstrapManager::getInstance()->process();
				lastBootstrap = aTick;
			}
		}
		
		if(aTick >= nextSearchTime)
		{
			SearchManager::getInstance()->processSearches();
			nextSearchTime = aTick + SEARCH_PROCESSTIME;
		}
		
		if(aTick >= nextSelfLookup)
		{
			// find myself in the network
			SearchManager::getInstance()->findNode(ClientManager::getInstance()->getMe()->getCID());
			nextSelfLookup = aTick + SELF_LOOKUP_TIMER;
		}
		
		if(aTick >= nextFirewallCheck)
		{
			DHT::getInstance()->setRequestFWCheck();
			nextFirewallCheck = aTick + FWCHECK_TIME;
		}
	}
	
	void TaskManager::on(TimerManagerListener::Minute, uint64_t aTick) throw()
	{
		Utils::cleanFlood();
		
		// remove dead nodes
		DHT::getInstance()->checkExpiration(aTick);
		IndexManager::getInstance()->checkExpiration(aTick);
		
		DHT::getInstance()->saveData();
	}
	
}