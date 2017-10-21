//////////////////////////////////////////////////////////////////////////
//
// main.cpp - Defines the entry point for the console application.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// This sample demonstrates how to perform simple transcoding
// to WMA or WMV.
//
////////////////////////////////////////////////////////////////////////// 

#include "Transcode.h"

int wmain(int argc, wchar_t* argv[])
{
    (void)HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

    if (argc != 3)
    {
        wprintf_s(
			L"Usage: %s input_file output_file\n"
			L"Available output file types are:\n", argv[0]);
		for(const auto& t : CTranscoder::fileTypes) {
			wprintf_s(L"  %s: %s\n", t.first.c_str(), t.second.description);
		}
        return 0;
    }

    const WCHAR* sInputFile = argv[1];  // Audio source file name
    const WCHAR* sOutputFile = argv[2];  // Output file name
    
    HRESULT hr = S_OK;

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    if (SUCCEEDED(hr))
    {
        hr = MFStartup(MF_VERSION);
    }

    if (SUCCEEDED(hr))
    {
        CTranscoder transcoder;

		// Get encoder attributes from output file extension.
		const CTranscoder::FileTypeAttr* pAttr;
		hr = transcoder.getFileType(sOutputFile, &pAttr);
		if(SUCCEEDED(hr)) {
			wprintf_s(L"Transcoding to %s\n", pAttr->description);
		}

		if(SUCCEEDED(hr)) {
			// Create a media source for the input file.
			hr = transcoder.OpenFile(sInputFile);
		}

        if (SUCCEEDED(hr) && !IsEqualGUID(pAttr->audioSubType, GUID_NULL))
        {
            wprintf_s(L"Opened file: %s.\n", sInputFile);

            //Configure the profile and build a topology.
            hr = transcoder.ConfigureAudioOutput(pAttr->audioSubType);
        }

        if (SUCCEEDED(hr) && !IsEqualGUID(pAttr->videoSubType, GUID_NULL))
        {
            hr = transcoder.ConfigureVideoOutput(pAttr->videoSubType);
        }
    
        if (SUCCEEDED(hr))
        {
            hr = transcoder.ConfigureContainer(pAttr->containerType);
        }

        //Transcode and generate the output file.

        if (SUCCEEDED(hr))
        {
            hr = transcoder.EncodeToFile(sOutputFile);
        }
    
        if (SUCCEEDED(hr))
        {
            wprintf_s(L"Output file created: %s\n", sOutputFile);
        }
    }

    MFShutdown();
    CoUninitialize();

    if (FAILED(hr))
    {
        wprintf_s(L"Could not create the output file (0x%X).\n", hr);
    }

    return 0;
}

