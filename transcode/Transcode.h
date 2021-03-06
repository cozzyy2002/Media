//////////////////////////////////////////////////////////////////////////
//
// Transcode.h
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

#pragma once

// Specify target platform version in project property.
//#define WINVER _WIN32_WINNT_WIN7
//#define _WIN32_WINNT _WIN32_WINNT_WIN8
#include <SDKDDKVer.h>

#include <stdio.h>
#include <assert.h>
#include <mfapi.h>
#include <mfidl.h>
#include <map>

template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}


class CTranscoder
{
public:
    CTranscoder();
    virtual ~CTranscoder();

    HRESULT OpenFile(const WCHAR *sURL);
    HRESULT ConfigureAudioOutput(REFGUID subType);
    HRESULT ConfigureVideoOutput(REFGUID subType);
    HRESULT ConfigureContainer(REFGUID containerType);
    HRESULT EncodeToFile(const WCHAR *sURL);

	struct FileTypeAttr {
		LPCWSTR description;
		GUID audioSubType;
		GUID videoSubType;
		GUID containerType;
	};

	static const std::map<std::wstring, CTranscoder::FileTypeAttr> fileTypes;
	HRESULT getFileType(LPCWSTR fileName, const FileTypeAttr** ppAttr);

private:

    HRESULT Shutdown();
    HRESULT Transcode();
    HRESULT Start();
	HRESULT onSessionTopologyStatus(IMFMediaEvent* e);
	HRESULT dumpTopology(IMFTopology* topology);

    IMFMediaSession*        m_pSession;
    IMFMediaSource*         m_pSource;
    IMFTopology*            m_pTopology;
    IMFTranscodeProfile*    m_pProfile;
};