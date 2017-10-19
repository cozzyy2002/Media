#include "MediaSource.h"
#include <stdio.h>
#include <propkey.h>
#include <propvarutil.h>
#include <mferror.h>
#include <typeinfo>

static bool isSameObject(IUnknown* a, IUnknown* b) { return a == b; }

/*
	Setup inner property keys.
	Index of each key in innerProperties is same as the index used by IPropertyStore::GetAt() method.
*/
HRESULT CMediaSource::setupInnerProperties(IPropertyStore* propertyStore)
{
	innerProperties.clear();
	DWORD cProps;
	auto hr = propertyStore->GetCount(&cProps);
	if(SUCCEEDED(hr)) {
		for(DWORD i = 0; i < cProps; i++) {
			PROPERTYKEY key;
			hr = propertyStore->GetAt(i, &key);
			if(FAILED(hr)) break;
			innerProperties.push_back(key);
		}
	}
	return hr;
}

// Returns whether inner object has the property.
// Guarantees that inner IPropertyStore object is available, if returns true.
bool CMediaSource::isInnerProperty(const PROPERTYKEY& key)
{
	if(innerPropertyStore) {
		for each(auto& k in innerProperties) {
			if(IsEqualPropertyKey(key, k)) return true;
		}
	}
	return false;
}

template<class I>
static void swapObject(I** pObj, void** ppv, CMediaSource* pThis)
{
	printf_s(__FUNCTION__ "(%s)\n", typeid((I*)*ppv).name());
	*pObj = (I*)*ppv;
	*ppv = (I*)pThis;
}

CMediaSource::CMediaSource(IMFMediaSource * inner)
	: inner(inner), innerGetService(nullptr), innerPropertyStore(nullptr)
{
	auto hr = inner->QueryInterface(IID_PPV_ARGS(&innerGetService));
	if(SUCCEEDED(hr)) {
		/*auto cRef =*/ innerGetService->Release();
		//wprintf_s(__FUNCTIONW__ L"(): this=0x%08p cRef=%d\n", (void*)this, cRef);
		hr = innerGetService->GetService(MF_PROPERTY_HANDLER_SERVICE, IID_PPV_ARGS(&innerPropertyStore));
		if(SUCCEEDED(hr)) {
			// If IPropertyStore is implemented by another object, set the object to CComPtr.
			if(!isSameObject(inner, innerPropertyStore)) _innerPropertyStore = innerPropertyStore;
			hr = setupInnerProperties(innerPropertyStore);
			printf_s("Inner Property count=%d, HRESULT=0x%x.\n", innerProperties.size(), hr);
		}
	}
}

#pragma region Implementation of IPropertyStore

HRESULT CMediaSource::GetCount(DWORD *cProps)
{
	*cProps = innerProperties.size() + 1;
	return S_OK;
}

HRESULT CMediaSource::GetAt(DWORD iProp, PROPERTYKEY *pkey)
{
	auto hr(S_OK);
	switch(iProp - innerProperties.size()) {
	case 0:
		*pkey = PKEY_Keywords;
		break;
	default:
		if(innerPropertyStore) {
			hr = innerPropertyStore->GetAt(iProp, pkey);
		} else {
			*pkey = PKEY_Null;
			hr = E_INVALIDARG;
		}
		break;
	}
	return hr;
}

HRESULT CMediaSource::GetValue(REFPROPERTYKEY key, PROPVARIANT *pv)
{
	auto hr(S_OK);
	if(isInnerProperty(key)) {
		// If inner object has the property, return it's value.
		hr = innerPropertyStore->GetValue(key, pv);
	} else if(IsEqualPropertyKey(PKEY_Keywords, key)) {
		hr = InitPropVariantFromString(L"This is my file", pv);
	} else {
		// Note: If the key is not present in IPropertyStore object, this method returns S_OK.
		pv->vt = VT_EMPTY;
	}
	return hr;
}

HRESULT CMediaSource::SetValue(REFPROPERTYKEY key, REFPROPVARIANT propvar)
{
	return isInnerProperty(key) ? innerPropertyStore->SetValue(key, propvar) : STG_E_ACCESSDENIED;
}

HRESULT CMediaSource::Commit(void)
{
	return innerPropertyStore ? innerPropertyStore->Commit() : STG_E_ACCESSDENIED;
}

#pragma endregion

#pragma region Implementation of IMFGetService

HRESULT CMediaSource::GetService(REFGUID guidService, REFIID riid, LPVOID *ppvObject)
{
	auto hr(MF_E_UNSUPPORTED_SERVICE);
	if(IsEqualGUID(MF_PROPERTY_HANDLER_SERVICE, guidService)) {
		// Return this pointer as IMFGetService object.
		hr = QueryInterface(riid, ppvObject);
	} else if(innerGetService) {
		hr = innerGetService->GetService(guidService, riid, ppvObject);
	}
	return hr;
}

#pragma endregion

HRESULT CMediaSource::QueryInterface(REFIID riid, void ** ppvObject)
{
#pragma warning(push)
#pragma warning(disable : 4838)
	static const QITAB qitab[] = {
		QITABENT(CMediaSource, IMFGetService),
		QITABENT(CMediaSource, IPropertyStore),
		{ 0 }
	};
#pragma warning(pop)

	auto hr = QISearch(this, qitab, riid, ppvObject);
	if(FAILED(hr)) {
		hr = inner->QueryInterface(riid, ppvObject);
	}
#if TEST
	CComHeapPtr<OLECHAR> str;
	if(SUCCEEDED(StringFromIID(riid, &str))) {
		wprintf_s(__FUNCTIONW__ L"(%s): HRESULT=0x%08x\n", str.m_pData, hr);
	}
#endif
	return hr;
}

ULONG CMediaSource::AddRef(void)
{
	auto cRef(inner->AddRef());
	//wprintf_s(__FUNCTIONW__ L"(): this=0x%08p cRef=%d\n", (void*)this, cRef);
	return cRef;
}

ULONG CMediaSource::Release(void)
{
	auto cRef(inner->Release());
	//wprintf_s(__FUNCTIONW__ L"(): this=0x%08p cRef=%d\n", (void*)this, cRef);
	if(cRef == 0) delete this;
	return cRef;
}
