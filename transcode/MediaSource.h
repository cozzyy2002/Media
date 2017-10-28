#pragma once
#include "Transcode.h"
#include <mfidl.h>
#include <Shlwapi.h>
#include <vector>
#include <atlbase.h>

class CMediaSource : public IMFMediaSource, public IMFGetService, public IPropertyStore
{
protected:
	// Inner object and CMediaSource share reference counter, so we have inner object as raw pointer.
	// Inner IMFMediaSource implements IMFGetService.
	IMFMediaSource* inner;
	IMFGetService* innerGetService;
	IPropertyStore* innerPropertyStore;
	CComPtr<IPropertyStore> _innerPropertyStore;
	std::vector<PROPERTYKEY> innerProperties;

	HRESULT setupInnerProperties(IPropertyStore* propertyStore);
	bool isInnerProperty(const PROPERTYKEY& key);

public:
	CMediaSource(IMFMediaSource* inner);
	~CMediaSource() {}

#pragma region Implementation of IPropertyStore
	virtual HRESULT STDMETHODCALLTYPE GetCount(
		/* [out] */ __RPC__out DWORD *cProps);
	virtual HRESULT STDMETHODCALLTYPE GetAt(
		/* [in] */ DWORD iProp,
		/* [out] */ __RPC__out PROPERTYKEY *pkey);
	virtual HRESULT STDMETHODCALLTYPE GetValue(
		/* [in] */ __RPC__in REFPROPERTYKEY key,
		/* [out] */ __RPC__out PROPVARIANT *pv);
	virtual HRESULT STDMETHODCALLTYPE SetValue(
		/* [in] */ __RPC__in REFPROPERTYKEY key,
		/* [in] */ __RPC__in REFPROPVARIANT propvar);
	virtual HRESULT STDMETHODCALLTYPE Commit(void);
#pragma endregion

#pragma region Implementation of IMFGetService
	virtual HRESULT STDMETHODCALLTYPE GetService(
		/* [in] */ __RPC__in REFGUID guidService,
		/* [in] */ __RPC__in REFIID riid,
		/* [iid_is][out] */ __RPC__deref_out_opt LPVOID *ppvObject);
#pragma endregion

#pragma region Implementation of IMFMediaSource
	virtual HRESULT STDMETHODCALLTYPE GetCharacteristics(
		/* [out] */ __RPC__out DWORD *pdwCharacteristics) {
		return inner->GetCharacteristics(pdwCharacteristics);
	}
	virtual /* [local] */ HRESULT STDMETHODCALLTYPE CreatePresentationDescriptor(
		/* [annotation][out] */
		_Outptr_  IMFPresentationDescriptor **ppPresentationDescriptor) {
		return inner->CreatePresentationDescriptor(ppPresentationDescriptor);
	}
	virtual HRESULT STDMETHODCALLTYPE Start(
		/* [in] */ __RPC__in_opt IMFPresentationDescriptor *pPresentationDescriptor,
		/* [unique][in] */ __RPC__in_opt const GUID *pguidTimeFormat,
		/* [unique][in] */ __RPC__in_opt const PROPVARIANT *pvarStartPosition) {
		return inner->Start(pPresentationDescriptor, pguidTimeFormat, pvarStartPosition);
	}
	virtual HRESULT STDMETHODCALLTYPE Stop(void) { return inner->Stop(); }
	virtual HRESULT STDMETHODCALLTYPE Pause(void) { return inner->Pause(); }
	virtual HRESULT STDMETHODCALLTYPE Shutdown(void) { return inner->Shutdown(); }
#pragma endregion

#pragma region Implementation of IMFEventGenerator
	virtual HRESULT STDMETHODCALLTYPE GetEvent(
		/* [in] */ DWORD dwFlags,
		/* [out] */ __RPC__deref_out_opt IMFMediaEvent **ppEvent) {
		return inner->GetEvent(dwFlags, ppEvent);
	}
	virtual /* [local] */ HRESULT STDMETHODCALLTYPE BeginGetEvent(
		/* [in] */ IMFAsyncCallback *pCallback,
		/* [in] */ IUnknown *punkState) {
		return inner->BeginGetEvent(pCallback, punkState);
	}
	virtual /* [local] */ HRESULT STDMETHODCALLTYPE EndGetEvent(
		/* [in] */ IMFAsyncResult *pResult,
		/* [annotation][out] */
		_Out_  IMFMediaEvent **ppEvent) {
		return inner->EndGetEvent(pResult, ppEvent);
	}
	virtual HRESULT STDMETHODCALLTYPE QueueEvent(
		/* [in] */ MediaEventType met,
		/* [in] */ __RPC__in REFGUID guidExtendedType,
		/* [in] */ HRESULT hrStatus,
		/* [unique][in] */ __RPC__in_opt const PROPVARIANT *pvValue) {
		return inner->QueueEvent(met, guidExtendedType, hrStatus, pvValue);
	}
#pragma endregion

#pragma region Implementation of IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject);
	virtual ULONG STDMETHODCALLTYPE AddRef(void);
	virtual ULONG STDMETHODCALLTYPE Release(void);
#pragma endregion
};
