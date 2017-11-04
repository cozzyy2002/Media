//////////////////////////////////////////////////////////////////////////
//
// Transcode.cpp
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
//
// This sample demonstrates how to perform simple transcoding
// to WMA or WMV.
//
////////////////////////////////////////////////////////////////////////// 


#include "Transcode.h"
#include "MediaSource.h"
#include <Shlwapi.h>
#include <locale>
#include <memory>
#include <propvarutil.h>
#include <mftransform.h>
#include <codecapi.h>

#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Propsys.lib")

using FileType = std::pair<std::wstring, CTranscoder::FileTypeAttr>;
static FileType fileTypeData[] = {
	{ L".wmv",{ L"Windows Media Video", MFAudioFormat_WMAudioV9, MFVideoFormat_WMV3, MFTranscodeContainerType_ASF } },
	{ L".wma",{ L"Windows Media Audio", MFAudioFormat_WMAudioV9, GUID_NULL, MFTranscodeContainerType_ASF } },
	{ L".mp4",{ L"MP4(H.264 video and AAC audio)", MFAudioFormat_AAC, MFVideoFormat_H264, MFTranscodeContainerType_MPEG4 } },
	{ L".aac",{ L"Advanced Audio Coding", MFAudioFormat_AAC, GUID_NULL, MFTranscodeContainerType_MPEG4 } },
};

template<typename T>
struct ValueName
{
	T value;
	LPCWSTR str;
};

template<typename T>
LPCWSTR valueToName(const ValueName<T>* valueNames, typename T value);

#define VALUE_NAME_ENT(v) { v, L#v }

static ValueName<REFGUID> g_AttributeValueNames[] = {
	VALUE_NAME_ENT(MF_ACTIVATE_MFT_LOCKED),
	VALUE_NAME_ENT(MF_SA_D3D_AWARE),
	VALUE_NAME_ENT(MF_TRANSFORM_ASYNC),
	VALUE_NAME_ENT(MF_TRANSFORM_ASYNC_UNLOCK),
	VALUE_NAME_ENT(MF_TRANSFORM_CATEGORY_Attribute),
	VALUE_NAME_ENT(MF_TRANSFORM_FLAGS_Attribute),
	VALUE_NAME_ENT(MFT_CODEC_MERIT_Attribute),
	VALUE_NAME_ENT(MFT_CONNECTED_STREAM_ATTRIBUTE),
	VALUE_NAME_ENT(MFT_CONNECTED_TO_HW_STREAM),
	VALUE_NAME_ENT(MFT_ENUM_HARDWARE_URL_Attribute),
	VALUE_NAME_ENT(MFT_ENUM_TRANSCODE_ONLY_ATTRIBUTE),
	VALUE_NAME_ENT(MFT_FIELDOFUSE_UNLOCK_Attribute),
	VALUE_NAME_ENT(MFT_FRIENDLY_NAME_Attribute),
	VALUE_NAME_ENT(MFT_INPUT_TYPES_Attributes),
	VALUE_NAME_ENT(MFT_OUTPUT_TYPES_Attributes),
	VALUE_NAME_ENT(MFT_PREFERRED_ENCODER_PROFILE),
	VALUE_NAME_ENT(MFT_PREFERRED_OUTPUTTYPE_Attribute),
	VALUE_NAME_ENT(MFT_PREFERRED_OUTPUTTYPE_Attribute),
	VALUE_NAME_ENT(MFT_PROCESS_LOCAL_Attribute),
	VALUE_NAME_ENT(MFT_SUPPORT_DYNAMIC_FORMAT_CHANGE),
#pragma region	Attributes supported Windows 8.1 or later.
	VALUE_NAME_ENT(MFT_DECODER_EXPOSE_OUTPUT_TYPES_IN_NATIVE_ORDER),
	VALUE_NAME_ENT(MFT_DECODER_FINAL_VIDEO_RESOLUTION_HINT),
	VALUE_NAME_ENT(MFT_ENCODER_SUPPORTS_CONFIG_EVENT),
	VALUE_NAME_ENT(MFT_ENUM_ADAPTER_LUID),
	VALUE_NAME_ENT(MFT_ENUM_HARDWARE_VENDOR_ID_Attribute),
	VALUE_NAME_ENT(MFT_REMUX_MARK_I_PICTURE_AS_CLEAN_POINT),
	VALUE_NAME_ENT(MFT_SUPPORT_3DVIDEO),
	VALUE_NAME_ENT(MFT_TRANSFORM_CLSID_Attribute),
	VALUE_NAME_ENT(MF_SA_REQUIRED_SAMPLE_COUNT_PROGRESSIVE),
	VALUE_NAME_ENT(MF_SA_MINIMUM_OUTPUT_SAMPLE_COUNT),
	VALUE_NAME_ENT(MF_SA_MINIMUM_OUTPUT_SAMPLE_COUNT_PROGRESSIVE),
	VALUE_NAME_ENT(MF_ENABLE_3DVIDEO_OUTPUT),
	VALUE_NAME_ENT(MF_SA_D3D11_BINDFLAGS),
	VALUE_NAME_ENT(MF_SA_D3D11_USAGE),
	VALUE_NAME_ENT(MF_SA_D3D11_AWARE),
	VALUE_NAME_ENT(MF_SA_D3D11_SHARED),
	VALUE_NAME_ENT(MF_LOW_LATENCY),
	VALUE_NAME_ENT(CODECAPI_AVDecVideoThumbnailGenerationMode),
	VALUE_NAME_ENT(CODECAPI_AVDecSoftwareDynamicFormatChange),
	VALUE_NAME_ENT(CODECAPI_AVDecNumWorkerThreads),
#pragma endregion
	{ GUID_NULL, nullptr }
};

HRESULT CreateMediaSource(const WCHAR *sURL, IMFMediaSource** ppMediaSource);

//-------------------------------------------------------------------
//  CTranscoder constructor
//-------------------------------------------------------------------

CTranscoder::CTranscoder() : 
    m_pSession(NULL),
    m_pSource(NULL),
    m_pTopology(NULL),
    m_pProfile(NULL)
{

}

//-------------------------------------------------------------------
//  CTranscoder destructor
//-------------------------------------------------------------------

CTranscoder::~CTranscoder()
{
    Shutdown();

    SafeRelease(&m_pProfile);
    SafeRelease(&m_pTopology);
    SafeRelease(&m_pSource);
    SafeRelease(&m_pSession);
}


//-------------------------------------------------------------------
//  OpenFile
//        
//  1. Creates a media source for the caller specified URL.
//  2. Creates the media session.
//  3. Creates a transcode profile to hold the stream and 
//     container attributes.
//
//  sURL: Input file URL.
//-------------------------------------------------------------------

HRESULT CTranscoder::OpenFile(const WCHAR *sURL)
{
    if (!sURL)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    // Create the media source.
	IMFMediaSource* pSource;
    hr = CreateMediaSource(sURL, &pSource);
	if(SUCCEEDED(hr)) {
		m_pSource = new CMediaSource(pSource);
	}

    //Create the media session.
    if (SUCCEEDED(hr))
    {
        hr = MFCreateMediaSession(NULL, &m_pSession);
    }    

    // Create an empty transcode profile.
    if (SUCCEEDED(hr))
    {
        hr = MFCreateTranscodeProfile(&m_pProfile);
    }
    return hr;
}



//-------------------------------------------------------------------
//  ConfigureAudioOutput
//        
//  Configures the audio stream attributes.  
//  These values are stored in the transcode profile.
//
//-------------------------------------------------------------------

HRESULT CTranscoder::ConfigureAudioOutput(REFGUID subType)
{
    assert (m_pProfile);

    HRESULT hr = S_OK;
    DWORD dwMTCount = 0;

    IMFCollection   *pAvailableTypes = NULL;
    IUnknown        *pUnkAudioType = NULL;
    IMFMediaType    *pAudioType = NULL;
    IMFAttributes   *pAudioAttrs = NULL;

    // Get the list of output formats supported by the Windows Media 
    // audio encoder.

    hr = MFTranscodeGetAudioOutputAvailableTypes(
        subType, 
        MFT_ENUM_FLAG_ALL, 
        NULL, 
        &pAvailableTypes
        );

    // Get the number of elements in the list.

    if (SUCCEEDED(hr))
    {
        hr = pAvailableTypes->GetElementCount( &dwMTCount );

        if (dwMTCount == 0)
        {
            hr = E_UNEXPECTED;
        }
    }

    // In this simple case, use the first media type in the collection.

    if (SUCCEEDED(hr))
    {
        hr = pAvailableTypes->GetElement(0, &pUnkAudioType);    
    }

    if (SUCCEEDED(hr))
    {
        hr = pUnkAudioType->QueryInterface(IID_PPV_ARGS(&pAudioType)); 
    }

    // Create a copy of the attribute store so that we can modify it safely.
    if (SUCCEEDED(hr))
    {
        hr = MFCreateAttributes(&pAudioAttrs, 0);     
    }

    if (SUCCEEDED(hr))
    {
        hr = pAudioType->CopyAllItems(pAudioAttrs);
    }

    // Set the encoder to be Windows Media audio encoder, so that the 
    // appropriate MFTs are added to the topology.

    if (SUCCEEDED(hr))
    {
        hr = pAudioAttrs->SetGUID(MF_MT_SUBTYPE, subType);
    }
    
    // Set the attribute store on the transcode profile.
    if (SUCCEEDED(hr))
    {
        hr = m_pProfile->SetAudioAttributes( pAudioAttrs );
    }

    SafeRelease(&pAvailableTypes);
    SafeRelease(&pAudioType);
    SafeRelease(&pUnkAudioType);
    SafeRelease(&pAudioAttrs);

    return hr;
}


//-------------------------------------------------------------------
//  ConfigureVideoOutput
//        
//  Configures the Video stream attributes.  
//  These values are stored in the transcode profile.
//
//-------------------------------------------------------------------

HRESULT CTranscoder::ConfigureVideoOutput(REFGUID subType)
{
    assert (m_pProfile);

    HRESULT hr = S_OK;

    IMFAttributes* pVideoAttrs = NULL;

    // Configure the video stream

    // Create a new attribute store.
    if (SUCCEEDED(hr))
    {
        hr = MFCreateAttributes( &pVideoAttrs, 5 );
    }

    // Set the encoder to be Windows Media video encoder, so that the appropriate MFTs are added to the topology.
    if (SUCCEEDED(hr))
    {
        hr = pVideoAttrs->SetGUID(MF_MT_SUBTYPE, subType);
    }

    // Set the frame rate.
    if (SUCCEEDED(hr))
    {
        hr = MFSetAttributeRatio(pVideoAttrs, MF_MT_FRAME_RATE, 30, 1);
    }

    //Set the frame size.
    if (SUCCEEDED(hr))
    {
        hr = MFSetAttributeSize(pVideoAttrs, MF_MT_FRAME_SIZE, 320, 240);   
    }

    //Set the pixel aspect ratio
    if (SUCCEEDED(hr))
    {
        hr = MFSetAttributeRatio(pVideoAttrs, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    }

    // Set the bit rate.
    if (SUCCEEDED(hr))
    {
        hr = pVideoAttrs->SetUINT32(MF_MT_AVG_BITRATE, 300000);
    }

    // Set the attribute store on the transcode profile.
    if (SUCCEEDED(hr))
    {
        hr = m_pProfile->SetVideoAttributes( pVideoAttrs );
    }

    SafeRelease(&pVideoAttrs);
    return hr;
}


//-------------------------------------------------------------------
//  ConfigureContainer
//        
//  Configures the container attributes.  
//  These values are stored in the transcode profile.
//  
//  Note: Setting the container type does not insert the required 
//  MFT node in the transcode topology. The MFT node is based on the 
//  stream settings stored in the transcode profile.
//-------------------------------------------------------------------

HRESULT CTranscoder::ConfigureContainer(REFGUID containerType)
{
    assert (m_pProfile);
    
    HRESULT hr = S_OK;
    
    IMFAttributes* pContainerAttrs = NULL;

    //Set container attributes
    hr = MFCreateAttributes( &pContainerAttrs, 2 );

    //Set the output container to be ASF type
    if (SUCCEEDED(hr))
    {
        hr = pContainerAttrs->SetGUID(
            MF_TRANSCODE_CONTAINERTYPE, 
            containerType
            );
    }

    // Use the default setting. Media Foundation will use the stream 
    // settings set in ConfigureAudioOutput and ConfigureVideoOutput.

    if (SUCCEEDED(hr))
    {
        hr = pContainerAttrs->SetUINT32(
            MF_TRANSCODE_ADJUST_PROFILE, 
            MF_TRANSCODE_ADJUST_PROFILE_DEFAULT
            );
    }

    //Set the attribute store on the transcode profile.
    if (SUCCEEDED(hr))
    {
        hr = m_pProfile->SetContainerAttributes(pContainerAttrs);
    }

    SafeRelease(&pContainerAttrs);
    return hr;
}

//-------------------------------------------------------------------
//  EncodeToFile
//        
//  Builds the transcode topology based on the input source,
//  configured transcode profile, and the output container settings.  
//-------------------------------------------------------------------
HRESULT CTranscoder::EncodeToFile(const WCHAR *sURL)
{
    assert (m_pSession);
    assert (m_pSource);
    assert (m_pProfile);
    
    if (!sURL)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    //Create the transcode topology
    hr = MFCreateTranscodeTopology( m_pSource, sURL, m_pProfile, &m_pTopology );

    // Set the topology on the media session.
    if (SUCCEEDED(hr))
    {
        hr = m_pSession->SetTopology(0, m_pTopology);
    }
    
    //Get media session events. This will start the encoding session.
    if (SUCCEEDED(hr))
    {
        hr = Transcode();
    }

    return hr;
}

//-------------------------------------------------------------------
//  Name: Transcode
//        
//  Start the encoding session by controlling the media session.
//  
//  The encoding starts when the media session raises the 
//  MESessionTopologySet event. The media session is closed after 
//  receiving MESessionEnded. The encoded file is finalized after 
//  the session is closed.
//
//  For simplicity, this sample uses the synchronous method  for 
//  getting media session events. 
//-------------------------------------------------------------------

HRESULT CTranscoder::Transcode()
{
    assert (m_pSession);
    
    IMFMediaEvent* pEvent = NULL;
    MediaEventType meType = MEUnknown;  // Event type

    HRESULT hr = S_OK;
    HRESULT hrStatus = S_OK;            // Event status

    //Get media session events synchronously
    while (meType != MESessionClosed)
    {
        hr = m_pSession->GetEvent(0, &pEvent);

        if (FAILED(hr)) { break; }

        // Get the event type.
        hr = pEvent->GetType(&meType);
        
        if (FAILED(hr)) { break; }

        hr = pEvent->GetStatus(&hrStatus);
        
        if (FAILED(hr)) { break; }

        if (FAILED(hrStatus))
        {
            wprintf_s(L"Failed. 0x%X error condition triggered this event.\n", hrStatus);
            hr = hrStatus;
            break;
        }

        switch (meType)
        {
		case MESessionTopologyStatus:
			onSessionTopologyStatus(pEvent);
			break;

        case MESessionTopologySet:
            hr = Start();
            if (SUCCEEDED(hr))
            {
                wprintf_s(L"Ready to start.\n");
            }
            break;

        case MESessionStarted:
            wprintf_s(L"Started encoding...\n");
            break;

        case MESessionEnded:
            hr = m_pSession->Close();
            if (SUCCEEDED(hr))
            {
                wprintf_s(L"Finished encoding.\n");
            }
            break;

        case MESessionClosed:
            wprintf_s(L"Output file created.\n");
            break;
        }

        if (FAILED(hr))
        {
            break;
        }

        SafeRelease(&pEvent);
    }

    SafeRelease(&pEvent);
    return hr;
}

// Safe pointer for PROPVARIANT.
struct CSafePropVariant {
	CSafePropVariant() { PropVariantInit(&var); }
	~CSafePropVariant() { PropVariantClear(&var); }
	PROPVARIANT* operator&() { return &var; }
	PROPVARIANT* operator->() { return &var; }
	operator PROPVARIANT&() { return var; }
	PROPVARIANT var;
};

#define HR_EXPECT_OK(exp) \
	checkHResult(exp, #exp, __FILE__, __LINE__)
#define HR_EXPECT(exp, hr) \
	HR_EXPECT_OK((exp) ? S_OK : hr)
#define HR_ASSERT_OK(exp) \
	do { HRESULT hr = HR_EXPECT_OK(exp); if(FAILED(hr)) return hr; } while(false)
#define HR_ASSERT(exp, hr) \
	HR_ASSERT_OK((exp) ? S_OK : hr)

static HRESULT checkHResult(HRESULT hr, const char* exp, const char* file, int line)
{
	if(FAILED(hr)) {
		printf_s("%s faild. error=0x%08x, at:\n%s(%d)\n", exp, hr, file, line);
	}
	return hr;
}

HRESULT CTranscoder::onSessionTopologyStatus(IMFMediaEvent * e)
{
	// On SessionTopologyStatus event, event value is IMFTopology object.
	CSafePropVariant var;
	HR_ASSERT_OK(e->GetValue(&var));
	HR_ASSERT(var->vt == VT_UNKNOWN, E_UNEXPECTED);
	HR_ASSERT(var->punkVal, E_UNEXPECTED);
	CComPtr<IMFTopology> topology;
	HR_ASSERT_OK(var->punkVal->QueryInterface(&topology));

	// Status attribute specifies the new status of the topology.
	UINT32 status;
	HR_ASSERT_OK(e->GetUINT32(MF_EVENT_TOPOLOGY_STATUS, &status));
	switch(status) {
	case MF_TOPOSTATUS_READY:
		dumpTopology(topology);
		break;
	default:
		break;
	}
	return S_OK;
}

HRESULT CTranscoder::dumpTopology(IMFTopology * topology)
{
	WORD nodeCount;
	HR_ASSERT_OK(topology->GetNodeCount(&nodeCount));
	wprintf_s(__FUNCTIONW__ L": Node count=%d\n", nodeCount);
	for(WORD i = 0; i < nodeCount; i++) {
		std::wstring name;
		std::wstring strType;
		CComPtr<IMFTopologyNode> node;
		HR_ASSERT_OK(topology->GetNode(i, &node));
		MF_TOPOLOGY_TYPE type;
		HR_ASSERT_OK(node->GetNodeType(&type));
		wprintf_s(L"Node %d: type=%d\n", i, (int)type);
		CComPtr<IUnknown> unk;
		if(FAILED(HR_EXPECT_OK(node->GetObject(&unk)))) continue;
		CComPtr<IMFAttributes> attr;
		switch(type) {
		case MF_TOPOLOGY_SOURCESTREAM_NODE:
			strType = L"Source stream";
			HR_EXPECT_OK(unk->QueryInterface(&attr));
			break;
		case MF_TOPOLOGY_OUTPUT_NODE:
			strType = L"Output";
			HR_EXPECT_OK(unk->QueryInterface(&attr));
			break;
		case MF_TOPOLOGY_TEE_NODE:
			strType = L"Tee";
			HR_EXPECT_OK(unk->QueryInterface(&attr));
			break;
		case MF_TOPOLOGY_TRANSFORM_NODE:
			strType = L"Transform";
			{
				CComPtr<IMFTransform> transform;
				HR_ASSERT_OK(unk->QueryInterface(&transform));
				HR_ASSERT_OK(transform->GetAttributes(&attr));
			}
			break;
		}
		if(!strType.empty()) {
			wprintf_s(L"  %2d: type=%d(%s): %s\n", i, type, strType.c_str(), name.c_str());
		}
		if(attr) {
			UINT32 attrCount;
			HR_ASSERT_OK(attr->GetCount(&attrCount));
			wprintf_s(L"    attributes: count=%d\n", attrCount);
			for(UINT32 j = 0; j < attrCount; j++) {
				GUID key;
				CSafePropVariant var;
				HR_ASSERT_OK(attr->GetItemByIndex(j, &key, &var));
				CComHeapPtr<WCHAR> strVar;
				HR_ASSERT_OK(PropVariantToStringAlloc(var, &strVar));
				CComBSTR strKey(key);
				LPCWSTR strKeyName = valueToName<REFGUID>(g_AttributeValueNames, key);
				wprintf_s(L"      %s:%s=%s\n", (LPCWSTR)strKey, strKeyName, (LPCWSTR)strVar);
			}
		}
	}
	return S_OK;
}


//-------------------------------------------------------------------
//  Start
//
//  Starts the encoding session.
//-------------------------------------------------------------------
HRESULT CTranscoder::Start()
{
    assert(m_pSession != NULL);

    HRESULT hr = S_OK;

    PROPVARIANT varStart;
    PropVariantInit(&varStart);

    hr = m_pSession->Start(&GUID_NULL, &varStart);

    if (FAILED(hr))
    {
        wprintf_s(L"Failed to start the session...\n");
    }
    return hr;
}

//-------------------------------------------------------------------
//  Shutdown
//
//  Handler for the MESessionClosed event.
//  Shuts down the media session and the media source.
//-------------------------------------------------------------------

HRESULT CTranscoder::Shutdown()
{
    HRESULT hr = S_OK;

    // Shut down the media source
    if (m_pSource)
    {
        hr = m_pSource->Shutdown();
    }

    // Shut down the media session. (Synchronous operation, no events.)
    if (SUCCEEDED(hr))
    {
        if (m_pSession)
        {
            hr = m_pSession->Shutdown();
        }
    }

    if (FAILED(hr))
    {
        wprintf_s(L"Failed to close the session...\n");
    }
    return hr;
}



///////////////////////////////////////////////////////////////////////
//  CreateMediaSource
//
//  Creates a media source from a URL.
///////////////////////////////////////////////////////////////////////

HRESULT CreateMediaSource(
    const WCHAR *sURL,  // The URL of the file to open.
    IMFMediaSource** ppMediaSource // Receives a pointer to the media source.
    )
{
    if (!sURL)
    {
        return E_INVALIDARG;
    }

    if (!ppMediaSource)
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    
    MF_OBJECT_TYPE ObjectType = MF_OBJECT_INVALID;

    IMFSourceResolver* pSourceResolver = NULL;
    IUnknown* pUnkSource = NULL;

    // Create the source resolver.
    hr = MFCreateSourceResolver(&pSourceResolver);


    // Use the source resolver to create the media source.
    
    if (SUCCEEDED(hr))
    {
        hr = pSourceResolver->CreateObjectFromURL(
            sURL,                       // URL of the source.
            MF_RESOLUTION_MEDIASOURCE,  // Create a source object.
            NULL,                       // Optional property store.
            &ObjectType,                // Receives the created object type. 
            &pUnkSource                 // Receives a pointer to the media source.
            );
    }

    // Get the IMFMediaSource from the IUnknown pointer.
    if (SUCCEEDED(hr))
    {
        hr = pUnkSource->QueryInterface(IID_PPV_ARGS(ppMediaSource));
    }

    SafeRelease(&pSourceResolver);
    SafeRelease(&pUnkSource);
    return hr;
}

/*static*/ std::map<std::wstring, CTranscoder::FileTypeAttr> const
CTranscoder::fileTypes(fileTypeData, &fileTypeData[ARRAYSIZE(fileTypeData)]);

HRESULT CTranscoder::getFileType(LPCWSTR fileName, const FileTypeAttr** ppAttr)
{
	std::wstring ext(PathFindExtensionW(fileName));
	std::locale loc;
	for(auto it = ext.begin(); it != ext.end(); it++) {
		*it = std::tolower<std::wstring::value_type>(*it, loc);
	}
	auto it(fileTypes.find(ext));
	if(it != fileTypes.end()) {
		*ppAttr = &it->second;
		return S_OK;
	} else {
		return E_NOT_SET;
	}
}

template<typename T>
LPCWSTR valueToName(const ValueName<T>* valueNames, typename T value)
{
	while(valueNames->str) {
		if(valueNames->value == value) return valueNames->str;
		valueNames++;
	}
	return L"<Unknown>";
}
