// RWLock.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <windows.h>
#include <iostream>

namespace
{

class CRwLock
{
public:

	CRwLock();

	void readLock();

	void writeLock();

	void readUnlock();

	void writeUnlock();

private:
	
	static const LONG WRITER_BIT = 1L << (sizeof(LONG) * 8 - 2);

	LONG mWritersReaders;
};

CRwLock::CRwLock()
: mWritersReaders(0)
{
}

void CRwLock::readLock()
{
	LONG i = WRITER_BIT;
	if(::InterlockedExchangeAdd(&mWritersReaders, 1) >= WRITER_BIT)
	{
		while(::InterlockedCompareExchange(&mWritersReaders, 0, 0) >= WRITER_BIT)
		{
			Sleep(0);
		}
	}	
}

void CRwLock::writeLock()
{
	while(::InterlockedCompareExchange(&mWritersReaders, WRITER_BIT, 0) != 0)
	{
		Sleep(0);
	}	
}

void CRwLock::readUnlock()
{
	::InterlockedExchangeAdd(&mWritersReaders, -1);
}

void CRwLock::writeUnlock()
{
	::InterlockedExchangeAdd(&mWritersReaders, -WRITER_BIT);
}

}
//////////////////////////////////////////////////////////////////////////

struct SRWLockAndData  
{
	SRWLockAndData()
	{
		mDataArray = new LONG[ARRAY_SIZE];
	}

	~SRWLockAndData()
	{
		delete [] mDataArray;
	}

	CRwLock mLock;
	LONG *mDataArray;
	static const LONG ARRAY_SIZE = 100; 
};

DWORD WINAPI TestRWLockThread(LPVOID lpParameter)
{
	SRWLockAndData *lockAndData = (SRWLockAndData*)lpParameter;

	for(LONG i = 0; i < 1000000; ++i)
	{
		if(i % 10 != 0)
		{
			lockAndData->mLock.readLock();

			LONG sum = 0;
			for (LONG j = 0; j < SRWLockAndData::ARRAY_SIZE; ++j)
			{
				sum += lockAndData->mDataArray[j];
			}

			lockAndData->mLock.readUnlock();
		}
		else
		{
			lockAndData->mLock.writeLock();

			delete [] lockAndData->mDataArray;
			lockAndData->mDataArray = new LONG[SRWLockAndData::ARRAY_SIZE];
			
			for (LONG j = 0; j < SRWLockAndData::ARRAY_SIZE; ++j)
			{
				lockAndData->mDataArray[j] = j;
			}

			lockAndData->mLock.writeUnlock();
		}
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////

struct SCriticalSectionAndData  
{
	SCriticalSectionAndData()
	{
		InitializeCriticalSection(&mLock);
		mDataArray = new LONG[ARRAY_SIZE];
	}

	~SCriticalSectionAndData()
	{
		DeleteCriticalSection(&mLock);
		delete [] mDataArray;
	}

	CRITICAL_SECTION mLock;
	LONG *mDataArray;
	static const LONG ARRAY_SIZE = 100; 
};

DWORD WINAPI TestCriticalSectionThread(LPVOID lpParameter)
{
	SCriticalSectionAndData *lockAndData = (SCriticalSectionAndData*)lpParameter;

	for(LONG i = 0; i < 1000000; ++i)
	{
		EnterCriticalSection(&lockAndData->mLock);

		if(i % 10 != 0)
		{
			LONG sum = 0;
			for (LONG j = 0; j < SCriticalSectionAndData::ARRAY_SIZE; ++j)
			{
				sum += lockAndData->mDataArray[j];
			}
		}
		else
		{
			delete [] lockAndData->mDataArray;
			lockAndData->mDataArray = new LONG[SCriticalSectionAndData::ARRAY_SIZE];

			for (LONG j = 0; j < SCriticalSectionAndData::ARRAY_SIZE; ++j)
			{
				lockAndData->mDataArray[j] = j;
			}
		}

		LeaveCriticalSection(&lockAndData->mLock);
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////

int _tmain(int argc, _TCHAR* argv[])
{
	const LONG THREAD_COUNT = 3;
	HANDLE threadHandles[THREAD_COUNT];

	//////////////////////////////////////////////////////////////////////////

	SRWLockAndData rwTestObject;

	for(LONG i = 0; i < THREAD_COUNT; ++i)
	{
		threadHandles[i] = CreateThread(NULL, 0, &TestRWLockThread, &rwTestObject, 0, NULL);
	}

	DWORD rwLockTime = GetTickCount();

	WaitForMultipleObjects(THREAD_COUNT, threadHandles, TRUE, INFINITE);

	rwLockTime = GetTickCount() - rwLockTime;

	for(LONG i = 0; i < THREAD_COUNT; ++i)
	{
		CloseHandle(threadHandles[i]);
	}

	//////////////////////////////////////////////////////////////////////////

	SCriticalSectionAndData csTestObject;

	for(LONG i = 0; i < THREAD_COUNT; ++i)
	{
		threadHandles[i] = CreateThread(NULL, 0, &TestCriticalSectionThread, &csTestObject, 0, NULL);
	}

	DWORD csLockTime = GetTickCount();

	WaitForMultipleObjects(THREAD_COUNT, threadHandles, TRUE, INFINITE);

	csLockTime = GetTickCount() - csLockTime;

	for(LONG i = 0; i < THREAD_COUNT; ++i)
	{
		CloseHandle(threadHandles[i]);
	}

	std::cout << "RWLock time " << rwLockTime << "ms" << std::endl;
	std::cout << "CriticalSection time " << csLockTime << "ms" << std::endl;

	return 0;
}

