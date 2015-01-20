#pragma once

#include "GothamTypes.h"
#include "GothamObj.h"

class GmSession : public GmObj
{
protected:
	GmSession(GmSessionType_t eType);
public:
	virtual ~GmSession();

	virtual _inline GmSessionType_t GetType()const
	{
		return m_eType;
	}

protected:

private:
	GmSessionType_t m_eType;
};
