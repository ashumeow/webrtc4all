#include "GothamMutex.h"
#include "GothamSinkUtils.h"

#include "tsk_mutex.h"
#include "tsk_debug.h"

GmMutex::GmMutex(bool bRecursive /*= true*/)
	: GmObj()
{
	m_phOTMutex = tsk_mutex_create_2(bRecursive ? tsk_true : tsk_false);
}

GmMutex::~GmMutex()
{
	if (m_phOTMutex)
	{
		tsk_mutex_destroy(&m_phOTMutex);
	}
	GM_DEBUG_INFO("*** GmMutex destroyed ***");
}

bool GmMutex::Lock()
{
	return (tsk_mutex_lock(m_phOTMutex) == 0);
}

bool GmMutex::Unlock()
{
	return (tsk_mutex_unlock(m_phOTMutex) == 0);
}
