#pragma warning(disable:4244)
#pragma warning(disable:4711)

#include <streams.h>
#include <stdio.h>
#include <olectl.h>
#include <dvdmedia.h>
#include "filters.h"
/*
struct {
	const char* name;
	int x, y;
	int avg; // 333333 = 30fps
} resolution[]{
{"QQVGA",	160,120,333333 },
{"QCIF",	176,144,333333 },
{"QCIF",	192,144,333333 },
{"HQVGA",	240,160,333333 },
{"QVGA",	320,240,333333 },
{"Video CD NTSC",	352,240 ,333333},
{"Video CD PAL",	352,288 ,333333},
{"xCIF",	384,288,333333	 },
{"360p",	480,360	,333333 },
{"nHD",	640,360	,333333 },
{"VGA",	640,480	,333333 },
{"SD",	704,480	,333333 },
{"DVD NTSC",	720,480	,333333 },
{"WGA",	800,480	,333333 },
{"SVGA",	800,600 ,333333},
{"DVCPRO HD",	960,720	,333333 },
{"XGA",	1024,768,333333	 },
{"HD",	1280,720	,333333 },
{"WXGA",	1280,800	,333333 },
{"SXGA−",	1280,960,333333	 },
{"SXGA",	1280,1024	,333333 },
{"UXGA",	1600,1200,333333	 },
{"FHD",	1920,1080,333333	 },
{"QXGA",	2048,1536,333333	 },
{"QSXGA",	2560,2048,333333	 },
{"QUXGA",	3200,2400,333333	 },
{"4K TV", 3840 , 2160,333333},
{"DCI 4K",	4096,2160,333333	 },
{"HXGA",	4096,3072,333333	 },*/
/*{"UW5K",	5120,2160,333333	 },
{"5K",	5120,2880,333333	 },
{"WHXGA",	5120,3200,333333	 },
{"HSXGA	",5120,4096,333333	 },
{"WHSXGA",	6400,4096,333333	 },
{"HUXGA",	6400,4800,333333	 },
{"8K UHD",	7680,4320,333333	 },
{"WHUXGA",	7680,4800,333333	 },
{"UW10K",	10240,4320	,333333 }*/

//};



#define BUF_SIZE (4096 * 3072 * 3)

//////////////////////////////////////////////////////////////////////////
//  CVCam is the source filter which masquerades as a capture device
//////////////////////////////////////////////////////////////////////////
CUnknown * WINAPI CVCam::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    ASSERT(phr);
    CUnknown *punk = new CVCam(lpunk, phr);
    return punk;
}

CVCam::CVCam(LPUNKNOWN lpunk, HRESULT *phr) : 
    CSource(NAME("Virtual Cam"), lpunk, CLSID_VirtualCam)
{
    ASSERT(phr);

	


    CAutoLock cAutoLock(&m_cStateLock);
    // Create the one and only output pin
    m_paStreams = (CSourceStream **) new CVCamStream*[1];
    m_paStreams[0] = new CVCamStream(phr, this, L"Video");
}

HRESULT CVCam::QueryInterface(REFIID riid, void **ppv) 
{
    //Forward request for IAMStreamConfig & IKsPropertySet to the pin
    if(riid == _uuidof(IAMStreamConfig) || riid == _uuidof(IKsPropertySet))
        return m_paStreams[0]->QueryInterface(riid, ppv);
    else
        return CSource::QueryInterface(riid, ppv);
}

//////////////////////////////////////////////////////////////////////////
// CVCamStream is the one and only output pin of CVCam which handles 
// all the stuff.
//////////////////////////////////////////////////////////////////////////

CVCamStream::CVCamStream(HRESULT *phr, CVCam *pParent, LPCWSTR pPinName) :
    CSourceStream(NAME("Video"),phr, pParent, pPinName), m_pParent(pParent)
{
	hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,BUF_SIZE, memnamedata);                 // name of mapping object
	if (hMapFile == NULL)
	{
		MessageBox(NULL, L"Could not open file mapping object", L"error", MB_OK);		
	}
	else {
		pBuf = (LPTSTR)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, BUF_SIZE);
	}
	if (pBuf == NULL)
	{
		MessageBox(NULL, L"Could not map view of file", L"error", MB_OK);
	}
	
	hMapFile2 = CreateFileMapping(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(BITMAPINFOHEADER), memnameconfig);                 // name of mapping object
	if (hMapFile2 == NULL)
	{
		MessageBox(NULL, L"Could not open file mapping object", L"error", MB_OK);
	}
	else {
		bi = (BITMAPINFOHEADER*)MapViewOfFile(hMapFile2, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(BITMAPINFOHEADER));
	}
	if (bi == NULL)
	{
		MessageBox(NULL, L"Could not map view of file", L"error", MB_OK);
	}

	m_hMutex = CreateMutex(NULL, FALSE, memnamelock);
	
	
	GetMediaType(0, &m_mt);
}


CVCamStream::~CVCamStream()
{
	UnmapViewOfFile(pBuf);
	CloseHandle(hMapFile);
	UnmapViewOfFile(bi);
	CloseHandle(hMapFile2);
	CloseHandle(m_hMutex);
} 

HRESULT CVCamStream::QueryInterface(REFIID riid, void **ppv)
{   
    // Standard OLE stuff
    if(riid == _uuidof(IAMStreamConfig))
        *ppv = (IAMStreamConfig*)this;
    else if(riid == _uuidof(IKsPropertySet))
        *ppv = (IKsPropertySet*)this;
    else
        return CSourceStream::QueryInterface(riid, ppv);

    AddRef();
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////
//  This is the routine where we create the data being output by the Virtual
//  Camera device.
//////////////////////////////////////////////////////////////////////////

HRESULT CVCamStream::FillBuffer(IMediaSample *pms)
{
    REFERENCE_TIME rtNow;
    
    REFERENCE_TIME avgFrameTime = ((VIDEOINFOHEADER*)m_mt.pbFormat)->AvgTimePerFrame;

    rtNow = m_rtLastTime;
    m_rtLastTime += avgFrameTime;
    pms->SetTime(&rtNow, &m_rtLastTime);
    pms->SetSyncPoint(TRUE);

    BYTE *pData;
    long lDataLen;
    pms->GetPointer(&pData);
    lDataLen = pms->GetSize();
	if (lDataLen > BUF_SIZE) {
		lDataLen = BUF_SIZE;
	}
	if (pBuf) {
		WaitForSingleObject(m_hMutex, INFINITE);
		memcpy(pData, pBuf, lDataLen);
		ReleaseMutex(m_hMutex);
	}

    return NOERROR;
} // FillBuffer


//
// Notify
// Ignore quality management messages sent from the downstream filter
STDMETHODIMP CVCamStream::Notify(IBaseFilter * pSender, Quality q)
{
    return E_NOTIMPL;
} // Notify

//////////////////////////////////////////////////////////////////////////
// This is called when the output format has been negotiated
//////////////////////////////////////////////////////////////////////////
HRESULT CVCamStream::SetMediaType(const CMediaType *pmt)
{
    DECLARE_PTR(VIDEOINFOHEADER, pvi, pmt->Format());
    HRESULT hr = CSourceStream::SetMediaType(pmt);
    return hr;
}

// See Directshow help topic for IAMStreamConfig for details on this method
HRESULT CVCamStream::GetMediaType(int iPosition, CMediaType *pmt)
{


    if(iPosition < 0) return E_INVALIDARG;
    if(iPosition >= resolutionsize) return VFW_S_NO_MORE_ITEMS;



    DECLARE_PTR(VIDEOINFOHEADER, pvi, pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER)));
    ZeroMemory(pvi, sizeof(VIDEOINFOHEADER));

    pvi->bmiHeader.biCompression = BI_RGB;
    pvi->bmiHeader.biBitCount    = 24;
    pvi->bmiHeader.biSize       = sizeof(BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth      = resolution[iPosition].x;
    pvi->bmiHeader.biHeight     = resolution[iPosition].y;
    pvi->bmiHeader.biPlanes     = 1;
    pvi->bmiHeader.biSizeImage  = GetBitmapSize(&pvi->bmiHeader);
    pvi->bmiHeader.biClrImportant = 0;

    pvi->AvgTimePerFrame = 1000000;

    SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
    SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle


    pmt->SetType(&MEDIATYPE_Video);
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetTemporalCompression(FALSE);

    // Work out the GUID for the subtype from the header info.
    const GUID SubTypeGUID = GetBitmapSubtype(&pvi->bmiHeader);
    pmt->SetSubtype(&SubTypeGUID);
    pmt->SetSampleSize(pvi->bmiHeader.biSizeImage);
    
    return NOERROR;

} // GetMediaType

// This method is called to see if a given output format is supported
HRESULT CVCamStream::CheckMediaType(const CMediaType *pMediaType)
{
    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)(pMediaType->Format());
    if(*pMediaType != m_mt) 
        return E_INVALIDARG;
    return S_OK;
} // CheckMediaType

// This method is called after the pins are connected to allocate buffers to stream data
HRESULT CVCamStream::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());
    HRESULT hr = NOERROR;

    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) m_mt.Format();
    pProperties->cBuffers = 1;
    pProperties->cbBuffer = pvi->bmiHeader.biSizeImage;

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties,&Actual);

    if(FAILED(hr)) return hr;
    if(Actual.cbBuffer < pProperties->cbBuffer) return E_FAIL;

    return NOERROR;
} // DecideBufferSize

// Called when graph is run
HRESULT CVCamStream::OnThreadCreate()
{
    m_rtLastTime = 0;
    return NOERROR;
} // OnThreadCreate


//////////////////////////////////////////////////////////////////////////
//  IAMStreamConfig
//////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CVCamStream::SetFormat(AM_MEDIA_TYPE* pmt)
{
	m_mt = *pmt;
	DECLARE_PTR(VIDEOINFOHEADER, pvi, m_mt.pbFormat);
	
	IPin* pin;
	ConnectedTo(&pin);
	if (pin)
	{
		IFilterGraph* pGraph = m_pParent->GetGraph();
		pGraph->Reconnect(this);
	}
	memcpy(bi, &(pvi->bmiHeader), sizeof(BITMAPINFOHEADER));
	//{char str[1024]; wsprintfA(str, "[ALEX] SetFormat %d %d %d\n", bi->biWidth, bi->biHeight, bi->biBitCount); OutputDebugStringA(str);}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CVCamStream::GetFormat(AM_MEDIA_TYPE **ppmt)
{
    *ppmt = CreateMediaType(&m_mt);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CVCamStream::GetNumberOfCapabilities(int *piCount, int *piSize)
{
    *piCount = resolutionsize;
    *piSize = sizeof(VIDEO_STREAM_CONFIG_CAPS);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CVCamStream::GetStreamCaps(int iIndex, AM_MEDIA_TYPE **pmt, BYTE *pSCC)
{

    *pmt = CreateMediaType(&m_mt);
    DECLARE_PTR(VIDEOINFOHEADER, pvi, (*pmt)->pbFormat);

    //if (iIndex == 0) iIndex = 3;

    pvi->bmiHeader.biCompression = BI_RGB;
    pvi->bmiHeader.biBitCount    = 24;
    pvi->bmiHeader.biSize       = sizeof(BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth      = resolution[iIndex].x;
    pvi->bmiHeader.biHeight     = resolution[iIndex].y;
    pvi->bmiHeader.biPlanes     = 1;
    pvi->bmiHeader.biSizeImage  = GetBitmapSize(&pvi->bmiHeader);
    pvi->bmiHeader.biClrImportant = 0;

    SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
    SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

    (*pmt)->majortype = MEDIATYPE_Video;
    (*pmt)->subtype = MEDIASUBTYPE_RGB24;
    (*pmt)->formattype = FORMAT_VideoInfo;
    (*pmt)->bTemporalCompression = FALSE;
    (*pmt)->bFixedSizeSamples= FALSE;
    (*pmt)->lSampleSize = pvi->bmiHeader.biSizeImage;
    (*pmt)->cbFormat = sizeof(VIDEOINFOHEADER);
    
    DECLARE_PTR(VIDEO_STREAM_CONFIG_CAPS, pvscc, pSCC);
    
    pvscc->guid = FORMAT_VideoInfo;
    pvscc->VideoStandard = AnalogVideo_None;
    pvscc->InputSize.cx = resolution[iIndex].x;
    pvscc->InputSize.cy = resolution[iIndex].y;
    pvscc->MinCroppingSize.cx = resolution[iIndex].x;
    pvscc->MinCroppingSize.cy = resolution[iIndex].y;
    pvscc->MaxCroppingSize.cx = resolution[iIndex].x;
    pvscc->MaxCroppingSize.cy = resolution[iIndex].y;
    pvscc->CropGranularityX = resolution[iIndex].x;
    pvscc->CropGranularityY = resolution[iIndex].y;
    pvscc->CropAlignX = 0;
    pvscc->CropAlignY = 0;

    pvscc->MinOutputSize.cx = resolution[iIndex].x;
    pvscc->MinOutputSize.cy = resolution[iIndex].y;
    pvscc->MaxOutputSize.cx = resolution[iIndex].x;
    pvscc->MaxOutputSize.cy = resolution[iIndex].y;
    pvscc->OutputGranularityX = 0;
    pvscc->OutputGranularityY = 0;
    pvscc->StretchTapsX = 0;
    pvscc->StretchTapsY = 0;
    pvscc->ShrinkTapsX = 0;
    pvscc->ShrinkTapsY = 0;

	pvscc->MinFrameInterval = resolution[iIndex].avg;
    pvscc->MaxFrameInterval = resolution[iIndex].avg;
    pvscc->MinBitsPerSecond = resolution[iIndex].x * resolution[iIndex].y * 3 * 8 * (10000000 / resolution[iIndex].avg);
    pvscc->MaxBitsPerSecond = resolution[iIndex].x * resolution[iIndex].y * 3 * 8 * (10000000 / resolution[iIndex].avg);

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
// IKsPropertySet
//////////////////////////////////////////////////////////////////////////


HRESULT CVCamStream::Set(REFGUID guidPropSet, DWORD dwID, void *pInstanceData, 
                        DWORD cbInstanceData, void *pPropData, DWORD cbPropData)
{// Set: Cannot set any properties.
    return E_NOTIMPL;
}

// Get: Return the pin category (our only property). 
HRESULT CVCamStream::Get(
    REFGUID guidPropSet,   // Which property set.
    DWORD dwPropID,        // Which property in that set.
    void *pInstanceData,   // Instance data (ignore).
    DWORD cbInstanceData,  // Size of the instance data (ignore).
    void *pPropData,       // Buffer to receive the property data.
    DWORD cbPropData,      // Size of the buffer.
    DWORD *pcbReturned     // Return the size of the property.
)
{
    if (guidPropSet != AMPROPSETID_Pin)             return E_PROP_SET_UNSUPPORTED;
    if (dwPropID != AMPROPERTY_PIN_CATEGORY)        return E_PROP_ID_UNSUPPORTED;
    if (pPropData == NULL && pcbReturned == NULL)   return E_POINTER;
    
    if (pcbReturned) *pcbReturned = sizeof(GUID);
    if (pPropData == NULL)          return S_OK; // Caller just wants to know the size. 
    if (cbPropData < sizeof(GUID))  return E_UNEXPECTED;// The buffer is too small.
        
    *(GUID *)pPropData = PIN_CATEGORY_CAPTURE;
    return S_OK;
}

// QuerySupported: Query whether the pin supports the specified property.
HRESULT CVCamStream::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport)
{
    if (guidPropSet != AMPROPSETID_Pin) return E_PROP_SET_UNSUPPORTED;
    if (dwPropID != AMPROPERTY_PIN_CATEGORY) return E_PROP_ID_UNSUPPORTED;
    // We support getting this property, but not setting it.
    if (pTypeSupport) *pTypeSupport = KSPROPERTY_SUPPORT_GET; 
    return S_OK;
}
