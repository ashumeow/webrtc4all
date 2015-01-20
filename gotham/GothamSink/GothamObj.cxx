#include "GothamObj.h"

#include "GothamSinkUtils.h"

#include <assert.h>

GmObj::GmObj()
	: m_nRefCount(1)
{

}

GmObj::~GmObj()
{
	GM_ASSERT(m_nRefCount == 0);
}

ULONG GmObj::AddRef()
{
	return InterlockedIncrement(&m_nRefCount);
}

ULONG  GmObj::Release()
{
	ULONG uCount = InterlockedDecrement(&m_nRefCount);
	if (uCount == 0)
	{
		delete this;
	}
	return uCount;
}

HRESULT GmObj::QueryInterface(REFIID iid, void** ppv)
{
	return E_NOINTERFACE;
}
