#ifndef PACER_H_INCLUDED
#define PACER_H_INCLUDED

#include "../GlobalDefinitions.h"

class CPacer
{
public:
	CPacer(uint64_t ns);
	~CPacer();
	uint64_t nstogo();
	void wait();
protected:
	uint64_t timekeeper;
	uint64_t interval;
#ifdef _WIN32
	HANDLE hTimer;
#endif
};
#endif
