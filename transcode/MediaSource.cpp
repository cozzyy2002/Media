#include "MediaSource.h"
#include <stdio.h>

HRESULT CMediaSource::GetCharacteristics(DWORD * pdwCharacteristics)
{
	return inner->GetCharacteristics(pdwCharacteristics);
}

HRESULT CMediaSource::CreatePresentationDescriptor(IMFPresentationDescriptor ** ppPresentationDescriptor)
{
	return inner->CreatePresentationDescriptor(ppPresentationDescriptor);
}

HRESULT CMediaSource::Start(IMFPresentationDescriptor * pPresentationDescriptor, const GUID * pguidTimeFormat, const PROPVARIANT * pvarStartPosition)
{
	return inner->Start(pPresentationDescriptor, pguidTimeFormat, pvarStartPosition);
}

HRESULT CMediaSource::Stop(void)
{
	return inner->Stop();
}

HRESULT CMediaSource::Pause(void)
{
	return inner->Pause();
}

HRESULT CMediaSource::Shutdown(void)
{
	return inner->Shutdown();
}

HRESULT CMediaSource::GetEvent(DWORD dwFlags, IMFMediaEvent ** ppEvent)
{
	return inner->GetEvent(dwFlags, ppEvent);
}

HRESULT CMediaSource::BeginGetEvent(IMFAsyncCallback * pCallback, IUnknown * punkState)
{
	return inner->BeginGetEvent(pCallback, punkState);
}

HRESULT CMediaSource::EndGetEvent(IMFAsyncResult * pResult, IMFMediaEvent ** ppEvent)
{
	return inner->EndGetEvent(pResult, ppEvent);
}

HRESULT CMediaSource::QueueEvent(MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT * pvValue)
{
	return inner->QueueEvent(met, guidExtendedType, hrStatus, pvValue);
}

HRESULT CMediaSource::QueryInterface(REFIID riid, void ** ppvObject)
{
	return inner->QueryInterface(riid, ppvObject);
}

ULONG CMediaSource::AddRef(void)
{
	auto cRef(inner->AddRef());
	//wprintf_s(__FUNCTIONW__ L"() : this=0x%08p cRef=%d\n", (void*)this, cRef);
	return cRef;
}

ULONG CMediaSource::Release(void)
{
	auto cRef(inner->Release());
	//wprintf_s(__FUNCTIONW__ L"(): this=0x%08p cRef=%d\n", (void*)this, cRef);
	if(cRef == 0) delete this;
	return cRef;
}
