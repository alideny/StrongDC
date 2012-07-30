/* 
 * Copyright (C) 2009-2010 Big Muscle
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

#include "DownloadManager.h"
#include "Socket.h"
#include "ThrottleManager.h"
#include "TimerManager.h"
#include "UploadManager.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread.hpp>
#include <boost/detail/lightweight_mutex.hpp>

namespace dcpp
{

	#define CONDWAIT_TIMEOUT		250
	#define MIN_UPLOAD_SPEED_LIMIT	5 * UploadManager::getInstance()->getSlots() + 4
	#define MAX_LIMIT_RATIO			7

	// constructor
	ThrottleManager::ThrottleManager(void) : downTokens(0), upTokens(0), downLimit(0), upLimit(0)
	{
		TimerManager::getInstance()->addListener(this);
	}

	// destructor
	ThrottleManager::~ThrottleManager(void)
	{
		TimerManager::getInstance()->removeListener(this);

		// release conditional variables on exit
		downCond.notify_all();
		upCond.notify_all();
	}

	/*
	 * Limits a traffic and reads a packet from the network
	 */
	int ThrottleManager::read(Socket* sock, void* buffer, size_t len)
	{
		size_t downs = DownloadManager::getInstance()->getDownloadCount();
		if(!BOOLSETTING(THROTTLE_ENABLE) || downLimit == 0 || downs == 0)
			return sock->read(buffer, len);

		boost::unique_lock<boost::mutex> lock(downMutex);


		if(downTokens > 0)
		{
			size_t slice = getDownloadLimit() / downs;
			size_t readSize = min(slice, min(len, downTokens));
				
			// read from socket
			readSize = sock->read(buffer, readSize);
				
			if(readSize > 0)
				downTokens -= readSize;

			// next code can't be in critical section, so we must unlock here
			lock.unlock();

			// give a chance to other transfers to get a token
			boost::thread::yield();
			return readSize;
		}

		// no tokens, wait for them
		downCond.timed_wait(lock, boost::posix_time::millisec(CONDWAIT_TIMEOUT));
		return -1;	// from BufferedSocket: -1 = retry, 0 = connection close
	}
	
	/*
	 * Limits a traffic and writes a packet to the network
	 * We must handle this a little bit differently than downloads, because of that stupidity in OpenSSL
	 */		
	int ThrottleManager::write(Socket* sock, void* buffer, size_t& len)
	{
		size_t ups = UploadManager::getInstance()->getUploadCount();
		if(!BOOLSETTING(THROTTLE_ENABLE) || upLimit == 0 || ups == 0)
			return sock->write(buffer, len);
		
		boost::unique_lock<boost::mutex> lock(upMutex);
		
		if(upTokens > 0)
		{
			size_t slice = getUploadLimit() / ups;
			len = min(slice, min(len, upTokens));
			upTokens -= len;

			// next code can't be in critical section, so we must unlock here
			lock.unlock();

			// write to socket			
			int sent = sock->write(buffer, len);

			// give a chance to other transfers to get a token
			boost::thread::yield();
			return sent;
		}
		
		// no tokens, wait for them
		upCond.timed_wait(lock, boost::posix_time::millisec(CONDWAIT_TIMEOUT));
		return 0;	// from BufferedSocket: -1 = failed, 0 = retry
	}
	
	/*
	 * Returns current download limit.
	 */
	size_t ThrottleManager::getDownloadLimit() const
	{
		return downLimit;
	}

	/*
	 * Returns current upload limit.
	 */
	size_t ThrottleManager::getUploadLimit() const
	{
		return upLimit;
	}

	// TimerManagerListener
	void ThrottleManager::on(TimerManagerListener::Second, uint64_t /*aTick*/) throw()
	{
		if(!BOOLSETTING(THROTTLE_ENABLE))
		{
			downLimit = 0;
			upLimit = 0;
			return;
		}
			
		// limiter restrictions: up_limit >= 5 * slots + 4, up_limit >= 7 * down_limit
		if(SETTING(MAX_UPLOAD_SPEED_LIMIT) < MIN_UPLOAD_SPEED_LIMIT)
		{
			SettingsManager::getInstance()->set(SettingsManager::MAX_UPLOAD_SPEED_LIMIT, MIN_UPLOAD_SPEED_LIMIT);
		}
			
		if((SETTING(MAX_DOWNLOAD_SPEED_LIMIT) > MAX_LIMIT_RATIO * SETTING(MAX_UPLOAD_SPEED_LIMIT) ) || (SETTING(MAX_DOWNLOAD_SPEED_LIMIT) == 0))
		{
			SettingsManager::getInstance()->set(SettingsManager::MAX_DOWNLOAD_SPEED_LIMIT, MAX_LIMIT_RATIO * SETTING(MAX_UPLOAD_SPEED_LIMIT) );
		}

		if(SETTING(MAX_UPLOAD_SPEED_LIMIT_TIME) < MIN_UPLOAD_SPEED_LIMIT)
		{
			SettingsManager::getInstance()->set(SettingsManager::MAX_UPLOAD_SPEED_LIMIT_TIME, MIN_UPLOAD_SPEED_LIMIT);
		}
			
		if((SETTING(MAX_DOWNLOAD_SPEED_LIMIT_TIME) > MAX_LIMIT_RATIO * SETTING(MAX_UPLOAD_SPEED_LIMIT_TIME)) || (SETTING(MAX_DOWNLOAD_SPEED_LIMIT_TIME) == 0)) 
		{
			SettingsManager::getInstance()->set(SettingsManager::MAX_DOWNLOAD_SPEED_LIMIT_TIME, MAX_LIMIT_RATIO * SETTING(MAX_UPLOAD_SPEED_LIMIT_TIME));
		}

		downLimit	= SETTING(MAX_DOWNLOAD_SPEED_LIMIT) * 1024;
		upLimit		= SETTING(MAX_UPLOAD_SPEED_LIMIT) * 1024;

		// alternative limiter
		if(BOOLSETTING(TIME_DEPENDENT_THROTTLE))
		{
			time_t currentTime;
			time(&currentTime);
			int currentHour = localtime(&currentTime)->tm_hour;

			if(	(SETTING(BANDWIDTH_LIMIT_START) < SETTING(BANDWIDTH_LIMIT_END) &&
				currentHour >= SETTING(BANDWIDTH_LIMIT_START) && currentHour < SETTING(BANDWIDTH_LIMIT_END)) ||
				(SETTING(BANDWIDTH_LIMIT_START) > SETTING(BANDWIDTH_LIMIT_END) &&
				(currentHour >= SETTING(BANDWIDTH_LIMIT_START) || currentHour < SETTING(BANDWIDTH_LIMIT_END))))
			{
				downLimit	= SETTING(MAX_DOWNLOAD_SPEED_LIMIT_TIME) * 1024;
				upLimit		= SETTING(MAX_UPLOAD_SPEED_LIMIT_TIME) * 1024;
			}
		}
		
		// readd tokens
		if(downLimit > 0)
		{
			boost::lock_guard<boost::mutex> lock(downMutex);
			downTokens = downLimit;
			downCond.notify_all();
		}
			
		if(upLimit > 0)
		{
			boost::lock_guard<boost::mutex> lock(upMutex);
			upTokens = upLimit;
			upCond.notify_all();
		}
	}


}	// namespace dcpp