#pragma once

#include "GothamObj.h"
#include "GothamTypes.h"

class GmMutex : public GmObj
{
	friend class GmObj;
private:
	GmMutex(bool bRecursive = true);
public:
	virtual ~GmMutex();
	bool Lock();
	bool Unlock();

private:
	GmNativeMutexHandle_t* m_phOTMutex;
};
