#pragma once

#include <assert.h>

// Helper class to implement IMFAttributes. 

// This is an abstract class; the derived class must implement the IUnknown 
// methods. This class is a wrapper for the standard attribute store provided 
// in Media Foundation.

// template parameter: 
// The interface you are implementing, either IMFAttributes or an interface 
// that inherits IMFAttributes, such as IMFActivate

template <class IFACE = IMFAttributes>
class CGmAttributes : public IFACE
{
protected:
	IMFAttributes *m_pAttributes;

	// This version of the constructor does not initialize the 
	// attribute store. The derived class must call Initialize() in 
	// its own constructor.
	CGmAttributes() : m_pAttributes(NULL)
	{
	}

	// This version of the constructor initializes the attribute 
	// store, but the derived class must pass an HRESULT parameter 
	// to the constructor.

	CGmAttributes(HRESULT& hr, UINT32 cInitialSize = 0) : m_pAttributes(NULL)
	{
		hr = Initialize(cInitialSize);
	}

	// The next version of the constructor uses a caller-provided 
	// implementation of IMFAttributes.

	// (Sometimes you want to delegate IMFAttributes calls to some 
	// other object that implements IMFAttributes, rather than using 
	// MFCreateAttributes.)

	CGmAttributes(HRESULT& hr, IUnknown *pUnk)
	{
		hr = Initialize(pUnk);
	}

	virtual ~CGmAttributes()
	{
		if (m_pAttributes)
		{
			m_pAttributes->Release();
		}
	}

	// Initializes the object by creating the standard Media Foundation attribute store.
	HRESULT Initialize(UINT32 cInitialSize = 0)
	{
		if (m_pAttributes == NULL)
		{
			return MFCreateAttributes(&m_pAttributes, cInitialSize);
		}
		else
		{
			return S_OK;
		}
	}

	// Initializes this object from a caller-provided attribute store.
	// pUnk: Pointer to an object that exposes IMFAttributes.
	HRESULT Initialize(IUnknown *pUnk)
	{
		if (m_pAttributes)
		{
			m_pAttributes->Release();
			m_pAttributes = NULL;
		}


		return pUnk->QueryInterface(IID_PPV_ARGS(&m_pAttributes));
	}

public:

	// IMFAttributes methods

	STDMETHODIMP GetItem(REFGUID guidKey, PROPVARIANT* pValue)
	{
		assert(m_pAttributes);
		return m_pAttributes->GetItem(guidKey, pValue);
	}

	STDMETHODIMP GetItemType(REFGUID guidKey, MF_ATTRIBUTE_TYPE* pType)
	{
		assert(m_pAttributes);
		return m_pAttributes->GetItemType(guidKey, pType);
	}

	STDMETHODIMP CompareItem(REFGUID guidKey, REFPROPVARIANT Value, BOOL* pbResult)
	{
		assert(m_pAttributes);
		return m_pAttributes->CompareItem(guidKey, Value, pbResult);
	}

	STDMETHODIMP Compare(
		IMFAttributes* pTheirs,
		MF_ATTRIBUTES_MATCH_TYPE MatchType,
		BOOL* pbResult
		)
	{
		assert(m_pAttributes);
		return m_pAttributes->Compare(pTheirs, MatchType, pbResult);
	}

	STDMETHODIMP GetUINT32(REFGUID guidKey, UINT32* punValue)
	{
		assert(m_pAttributes);
		return m_pAttributes->GetUINT32(guidKey, punValue);
	}

	STDMETHODIMP GetUINT64(REFGUID guidKey, UINT64* punValue)
	{
		assert(m_pAttributes);
		return m_pAttributes->GetUINT64(guidKey, punValue);
	}

	STDMETHODIMP GetDouble(REFGUID guidKey, double* pfValue)
	{
		assert(m_pAttributes);
		return m_pAttributes->GetDouble(guidKey, pfValue);
	}

	STDMETHODIMP GetGUID(REFGUID guidKey, GUID* pguidValue)
	{
		assert(m_pAttributes);
		return m_pAttributes->GetGUID(guidKey, pguidValue);
	}

	STDMETHODIMP GetStringLength(REFGUID guidKey, UINT32* pcchLength)
	{
		assert(m_pAttributes);
		return m_pAttributes->GetStringLength(guidKey, pcchLength);
	}

	STDMETHODIMP GetString(REFGUID guidKey, LPWSTR pwszValue, UINT32 cchBufSize, UINT32* pcchLength)
	{
		assert(m_pAttributes);
		return m_pAttributes->GetString(guidKey, pwszValue, cchBufSize, pcchLength);
	}

	STDMETHODIMP GetAllocatedString(REFGUID guidKey, LPWSTR* ppwszValue, UINT32* pcchLength)
	{
		assert(m_pAttributes);
		return m_pAttributes->GetAllocatedString(guidKey, ppwszValue, pcchLength);
	}

	STDMETHODIMP GetBlobSize(REFGUID guidKey, UINT32* pcbBlobSize)
	{
		assert(m_pAttributes);
		return m_pAttributes->GetBlobSize(guidKey, pcbBlobSize);
	}

	STDMETHODIMP GetBlob(REFGUID guidKey, UINT8* pBuf, UINT32 cbBufSize, UINT32* pcbBlobSize)
	{
		assert(m_pAttributes);
		return m_pAttributes->GetBlob(guidKey, pBuf, cbBufSize, pcbBlobSize);
	}

	STDMETHODIMP GetAllocatedBlob(REFGUID guidKey, UINT8** ppBuf, UINT32* pcbSize)
	{
		assert(m_pAttributes);
		return m_pAttributes->GetAllocatedBlob(guidKey, ppBuf, pcbSize);
	}

	STDMETHODIMP GetUnknown(REFGUID guidKey, REFIID riid, LPVOID* ppv)
	{
		assert(m_pAttributes);
		return m_pAttributes->GetUnknown(guidKey, riid, ppv);
	}

	STDMETHODIMP SetItem(REFGUID guidKey, REFPROPVARIANT Value)
	{
		assert(m_pAttributes);
		return m_pAttributes->SetItem(guidKey, Value);
	}

	STDMETHODIMP DeleteItem(REFGUID guidKey)
	{
		assert(m_pAttributes);
		return m_pAttributes->DeleteItem(guidKey);
	}

	STDMETHODIMP DeleteAllItems()
	{
		assert(m_pAttributes);
		return m_pAttributes->DeleteAllItems();
	}

	STDMETHODIMP SetUINT32(REFGUID guidKey, UINT32 unValue)
	{
		assert(m_pAttributes);
		return m_pAttributes->SetUINT32(guidKey, unValue);
	}

	STDMETHODIMP SetUINT64(REFGUID guidKey, UINT64 unValue)
	{
		assert(m_pAttributes);
		return m_pAttributes->SetUINT64(guidKey, unValue);
	}

	STDMETHODIMP SetDouble(REFGUID guidKey, double fValue)
	{
		assert(m_pAttributes);
		return m_pAttributes->SetDouble(guidKey, fValue);
	}

	STDMETHODIMP SetGUID(REFGUID guidKey, REFGUID guidValue)
	{
		assert(m_pAttributes);
		return m_pAttributes->SetGUID(guidKey, guidValue);
	}

	STDMETHODIMP SetString(REFGUID guidKey, LPCWSTR wszValue)
	{
		assert(m_pAttributes);
		return m_pAttributes->SetString(guidKey, wszValue);
	}

	STDMETHODIMP SetBlob(REFGUID guidKey, const UINT8* pBuf, UINT32 cbBufSize)
	{
		assert(m_pAttributes);
		return m_pAttributes->SetBlob(guidKey, pBuf, cbBufSize);
	}

	STDMETHODIMP SetUnknown(REFGUID guidKey, IUnknown* pUnknown)
	{
		assert(m_pAttributes);
		return m_pAttributes->SetUnknown(guidKey, pUnknown);
	}

	STDMETHODIMP LockStore()
	{
		assert(m_pAttributes);
		return m_pAttributes->LockStore();
	}

	STDMETHODIMP UnlockStore()
	{
		assert(m_pAttributes);
		return m_pAttributes->UnlockStore();
	}

	STDMETHODIMP GetCount(UINT32* pcItems)
	{
		assert(m_pAttributes);
		return m_pAttributes->GetCount(pcItems);
	}

	STDMETHODIMP GetItemByIndex(UINT32 unIndex, GUID* pguidKey, PROPVARIANT* pValue)
	{
		assert(m_pAttributes);
		return m_pAttributes->GetItemByIndex(unIndex, pguidKey, pValue);
	}

	STDMETHODIMP CopyAllItems(IMFAttributes* pDest)
	{
		assert(m_pAttributes);
		return m_pAttributes->CopyAllItems(pDest);
	}

	// Helper functions

	HRESULT SerializeToStream(DWORD dwOptions, IStream* pStm)
		// dwOptions: Flags from MF_ATTRIBUTE_SERIALIZE_OPTIONS
	{
		assert(m_pAttributes);
		return MFSerializeAttributesToStream(m_pAttributes, dwOptions, pStm);
	}

	HRESULT DeserializeFromStream(DWORD dwOptions, IStream* pStm)
	{
		assert(m_pAttributes);
		return MFDeserializeAttributesFromStream(m_pAttributes, dwOptions, pStm);
	}

	// SerializeToBlob: Stores the attributes in a byte array. 
	// 
	// ppBuf: Receives a pointer to the byte array. 
	// pcbSize: Receives the size of the byte array.
	//
	// The caller must free the array using CoTaskMemFree.
	HRESULT SerializeToBlob(UINT8 **ppBuffer, UINT32 *pcbSize)
	{
		assert(m_pAttributes);

		if (ppBuffer == NULL)
		{
			return E_POINTER;
		}
		if (pcbSize == NULL)
		{
			return E_POINTER;
		}

		*ppBuffer = NULL;
		*pcbSize = 0;

		UINT32 cbSize = 0;
		BYTE *pBuffer = NULL;

		HRESULT hr = MFGetAttributesAsBlobSize(m_pAttributes, &cbSize);

		if (FAILED(hr))
		{
			return hr;
		}

		pBuffer = (BYTE*)CoTaskMemAlloc(cbSize);
		if (pBuffer == NULL)
		{
			return E_OUTOFMEMORY;
		}

		hr = MFGetAttributesAsBlob(m_pAttributes, pBuffer, cbSize);

		if (SUCCEEDED(hr))
		{
			*ppBuffer = pBuffer;
			*pcbSize = cbSize;
		}
		else
		{
			CoTaskMemFree(pBuffer);
		}
		return hr;
	}

	HRESULT DeserializeFromBlob(const UINT8* pBuffer, UINT cbSize)
	{
		assert(m_pAttributes);
		return MFInitAttributesFromBlob(m_pAttributes, pBuffer, cbSize);
	}

	HRESULT GetRatio(REFGUID guidKey, UINT32* pnNumerator, UINT32* punDenominator)
	{
		assert(m_pAttributes);
		return MFGetAttributeRatio(m_pAttributes, guidKey, pnNumerator, punDenominator);
	}

	HRESULT SetRatio(REFGUID guidKey, UINT32 unNumerator, UINT32 unDenominator)
	{
		assert(m_pAttributes);
		return MFSetAttributeRatio(m_pAttributes, guidKey, unNumerator, unDenominator);
	}

	// Gets an attribute whose value represents the size of something (eg a video frame).
	HRESULT GetSize(REFGUID guidKey, UINT32* punWidth, UINT32* punHeight)
	{
		assert(m_pAttributes);
		return MFGetAttributeSize(m_pAttributes, guidKey, punWidth, punHeight);
	}

	// Sets an attribute whose value represents the size of something (eg a video frame).
	HRESULT SetSize(REFGUID guidKey, UINT32 unWidth, UINT32 unHeight)
	{
		assert(m_pAttributes);
		return MFSetAttributeSize(m_pAttributes, guidKey, unWidth, unHeight);
	}
};