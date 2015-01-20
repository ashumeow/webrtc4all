#pragma once

#include <windows.h>

MIDL_INTERFACE("F47C247C-B94A-4227-84B1-7986E5FFA81E")
IGmStreamWebRTC : public IUnknown
{
	virtual STDMETHODIMP Test() = 0;
};

MIDL_INTERFACE("9559B840-0092-433C-99F0-D072ED81183A")
IGmSessionWebRTC : public IUnknown
{
	virtual STDMETHODIMP Test() = 0;
};
