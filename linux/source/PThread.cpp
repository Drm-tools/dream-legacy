/* (C) Arturs Aboltins, Riga Technical University, 2010 */

extern "C" 
{
#include <sys/types.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
}

#include <iostream>

//local
#include "PThread.h"
#include "../common/GlobalDefinitions.h"

PThread::PThread()
{
	bRunning=false;
}

void PThread::start()
{
	int msg_size=128;
	char msg[msg_size];

	pthread_attr_t threadAttr;
	pthread_attr_init(&threadAttr);	
	errno = pthread_attr_setdetachstate(&threadAttr,PTHREAD_CREATE_DETACHED);
	if (errno)
	{
		snprintf(msg,msg_size,"pthread_attr_setdetachstate error");
        throw CGenErr(msg);
	}
	  
	errno = pthread_attr_setinheritsched(&threadAttr, PTHREAD_INHERIT_SCHED);
	if (errno)
	{
		snprintf(msg,msg_size,"pthread_attr_setinheritsched error");
        throw CGenErr(msg);
	}
	
	errno = pthread_attr_setstacksize(&threadAttr, DRM_STACKSIZE);
	if (errno)
	{
		snprintf(msg,msg_size,"pthread_attr_setstacksize error");
        throw CGenErr(msg);
	}
	
	pthread_create(&handle, &threadAttr, RunPThread, this);
	bRunning = true;
}

void PThread::cancel()
{
	pthread_cancel(this->handle);
	bRunning = false;
}

bool PThread::wait(int ms) 
{
	timespec ts;
	ts.tv_sec = ms/1000;	/* seconds */
	ts.tv_nsec = (ms%1000)*1000;   /* nanoseconds */

	int err = pthread_timedjoin_np(this->handle, NULL, &ts);
	//false if timed out
	return (err==ETIMEDOUT)?false:true;
};

void PThread::msleep(int ms) 
{
	::usleep(ms*1000L);
};

bool PThread::running() 
{
    return bRunning;
}

//--------------------------------------------------------------------------------------
void *RunPThread(void *pData)
{
  PThread *pThread=(PThread *)pData;
#ifdef _DEBUG_  
    cerr<<"Running thread 0x"<<hex<<pThread->handle<<endl; 
#endif
  pThread->run(); 
  return (void*)NULL;
}

//--------------------------------------------------------------------------------------
PMutex::PMutex()
{
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&m_Lock, &attr);		
}

PMutex::~PMutex()
{
	pthread_mutex_destroy(&m_Lock);		
}

void PMutex::Lock()
{
	pthread_mutex_lock(&m_Lock);
}

void PMutex::Unlock()
{
	pthread_mutex_unlock(&m_Lock);
}

int PMutex::TryLock()
{
	return pthread_mutex_trylock(&m_Lock);
}

pthread_mutex_t& PMutex::get_Mutex()
{
	return m_Lock;
}

PMutex::operator pthread_mutex_t()
{
	return m_Lock;
}

