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

#ifndef DCPLUSPLUS_DCPP_THREAD_H
#define DCPLUSPLUS_DCPP_THREAD_H

#ifndef _WIN32
#include <pthread.h>
#include <sched.h>
#include <sys/resource.h>
#endif

#include "Exception.h"

#include <boost/thread.hpp>

namespace dcpp {

STANDARD_EXCEPTION(ThreadException);

typedef boost::recursive_mutex	CriticalSection;
typedef boost::detail::spinlock	FastCriticalSection;
typedef boost::lock_guard<boost::recursive_mutex> Lock;
typedef boost::lock_guard<boost::detail::spinlock> FastLock;

class Thread : private boost::noncopyable
{
public:
#ifdef _WIN32
	enum Priority {
		IDLE = THREAD_PRIORITY_IDLE,
		LOW = THREAD_PRIORITY_BELOW_NORMAL,
		NORMAL = THREAD_PRIORITY_NORMAL,
		HIGH = THREAD_PRIORITY_ABOVE_NORMAL
	};

	Thread() throw() : threadHandle(INVALID_HANDLE_VALUE) { }
	virtual ~Thread() { 
		if(threadHandle != INVALID_HANDLE_VALUE)
			CloseHandle(threadHandle);
	}
	
	void start() throw(ThreadException);
	void join() throw(ThreadException) {
		if(threadHandle == INVALID_HANDLE_VALUE) {
			return;
		}

		WaitForSingleObject(threadHandle, INFINITE);
		CloseHandle(threadHandle);
		threadHandle = INVALID_HANDLE_VALUE;
	}

	void setThreadPriority(Priority p) throw() { ::SetThreadPriority(threadHandle, p); }
	
	static void sleep(uint64_t millis) { ::Sleep(static_cast<DWORD>(millis)); }
	static void yield() { ::Sleep(0); }
	
#else

	enum Priority {
		IDLE = 1,
		LOW = 1,
		NORMAL = 0,
		HIGH = -1
	};
	Thread() throw() : threadHandle(0) { }
	virtual ~Thread() { 
		if(threadHandle != 0) {
			pthread_detach(threadHandle);
		}
	}
	void start() throw(ThreadException);
	void join() throw() { 
		if (threadHandle) {
			pthread_join(threadHandle, 0);
			threadHandle = 0;
		}
	}

	void setThreadPriority(Priority p) { setpriority(PRIO_PROCESS, 0, p); }
	static void sleep(uint32_t millis) { ::usleep(millis*1000); }
	static void yield() { ::sched_yield(); }
#endif

protected:
	virtual int run() = 0;
	
#ifdef _WIN32
/*
	static void DbgDumpStack()
	{
	#ifdef _DEBUG
		OutputDebugString(_T("### Stack Dump Start\n"));
		PBYTE pPtr;
		_asm mov pPtr, esp; // Get stack pointer.
		// Get the stack last page.
		MEMORY_BASIC_INFORMATION stMemBasicInfo;
		VirtualQuery(pPtr, &stMemBasicInfo, sizeof(stMemBasicInfo));
		PBYTE pPos = (PBYTE) stMemBasicInfo.AllocationBase;
		size_t stackSize = 0;
		do
		{
			VirtualQuery(pPos, &stMemBasicInfo, sizeof(stMemBasicInfo));
			pPos += stMemBasicInfo.RegionSize;
			stackSize += stMemBasicInfo.RegionSize;

		} while (pPos < pPtr);
		TCHAR szTxt[0x100];
		wsprintf(szTxt, _T("Stack size: %s\n"), Util::formatBytesW(stackSize));
		OutputDebugString(szTxt);    
		OutputDebugString(_T("### Stack Dump Finish\n"));
	#endif // _DEBUG
	}
*/
	HANDLE threadHandle;

	static unsigned int  WINAPI starter(void* p) {
		Thread* t = (Thread*)p;
		//DbgDumpStack();
		t->run();
		return 0;
	}
#else
	pthread_t threadHandle;
	static void* starter(void* p) {
		Thread* t = (Thread*)p;
		t->run();
		return NULL;
	}
#endif
};

} // namespace dcpp

#endif // !defined(THREAD_H)

/**
 * @file
 * $Id$
 */
