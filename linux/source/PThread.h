/* (C) Arturs Aboltins, Riga Technical University, 2010 */

#ifndef _PTHREAD_H_d929a7c9f935a26b8f86c96046afef30
#define _PTHREAD_H_d929a7c9f935a26b8f86c96046afef30

extern "C" 
{
#include <sys/types.h>
#include <pthread.h>
}

#include <map>
#include <string>
using namespace std;

#define DRM_STACKSIZE 256 * 1024

class PThread 
{
    public:	
		PThread();
		virtual ~PThread() {};
		virtual void run()=0;
		
		void 	start();
		void 	cancel();
		bool 	wait(int);
		void 	msleep(int);
		bool 	running(); 
		
		friend void *RunPThread(void *pData);

	private:	
		pthread_t handle;
		bool bRunning;
};

void *RunPThread(void *pData);

class PMutex
{
protected:
	pthread_mutex_t m_Lock;	

public:
	PMutex();
	~PMutex();
	void Lock();
	void Unlock();
	int TryLock();

	pthread_mutex_t& get_Mutex();
	operator pthread_mutex_t();
};

//-----------------------------------------------------------------

template<class T>
class AutoLock
{
private:
  T &m_lock;

public:
  AutoLock(T &lock) : m_lock(lock) { m_lock.Lock(); }
  ~AutoLock() { m_lock.Unlock(); }
};

#endif /* _PTHREAD_H_d929a7c9f935a26b8f86c96046afef30 */


