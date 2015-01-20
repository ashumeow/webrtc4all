#pragma once

#include <atlbase.h>

class GmObj : public IUnknown
{
protected:
	GmObj();
public:
	virtual ~GmObj();

	// IUnknown
	STDMETHODIMP QueryInterface(REFIID iid, void** ppv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

#if 0
	template <class T>
	static HRESULT CreateInstanceWithRef(T** ppObject)
	{
		CComObject<T> *pObject;
		HRESULT hr = CComObject<T>::CreateInstance(&pObject);
		if (SUCCEEDED(hr))
		{
			pObject->AddRef();
			*ppObject = pObject;
		}
		return hr;
	}
#endif

	template <class T>
	static CComPtr<T> NewArg0() // Create Object with RefCount==1
	{
		T* _t = new T(); CComPtr<T>t = _t; if (_t)_t->Release(); return t;
	}

private:
	long m_nRefCount;
};
