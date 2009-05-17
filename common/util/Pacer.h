#ifndef PACER_H_INCLUDED
#define PACER_H_INCLUDED

#include "../GlobalDefinitions.h"

#ifdef _WIN32
# ifdef _WIN32_WINNT
#  undef _WIN32_WINNT
# endif
# define _WIN32_WINNT 0x0400
# include <windows.h>
# include <qnamespace.h>
#endif

class CPacer
{
public:
	CPacer(uint64_t ns);
	~CPacer();
	uint64_t nstogo();
	void wait();
	void changeInterval(uint64_t ns) { interval = ns; }
protected:
	uint64_t timekeeper;
	uint64_t interval;
#ifdef _WIN32
	Qt::HANDLE hTimer;
#endif
};
#endif
