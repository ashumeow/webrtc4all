#pragma once

#include "GothamObj.h"

extern const struct tmedia_producer_plugin_def_s *gm_producer_plugin_def_t;

class GmProducer : public GmObj
{
public:
	GmProducer(const struct gm_producer_s* pcWrappedProd);
	virtual ~GmProducer();

	HRESULT SetInputSize(UINT32 nWidth, UINT32 nHeight);
	HRESULT SetInputFormat(const GUID& subType);
	HRESULT WriteSamples(const BYTE* pBytesPtr, UINT32 nBytesCount);

private:
	const struct gm_producer_s* m_pcWrappedProd;
};