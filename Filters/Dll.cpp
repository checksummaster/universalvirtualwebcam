//////////////////////////////////////////////////////////////////////////
//  This file contains routines to register / Unregister the 
//  Directshow filter 'Virtual Cam'
//  We do not use the inbuilt BaseClasses routines as we need to register as
//  a capture source
//////////////////////////////////////////////////////////////////////////
#pragma comment(lib, "kernel32")
#pragma comment(lib, "user32")
#pragma comment(lib, "gdi32")
#pragma comment(lib, "advapi32")
#pragma comment(lib, "winmm")
#pragma comment(lib, "ole32")
#pragma comment(lib, "oleaut32")
#pragma comment(lib, "Shlwapi")
/*
#ifdef _DEBUG
	#pragma comment(lib, "strmbasd")
#else
	#pragma comment(lib, "strmbase")
#endif
*/

#include <streams.h>
#include <olectl.h>
#include <initguid.h>
#include <dllsetup.h>
#include <shlwapi.h>
#include <stdio.h>
#include "filters.h"
#include "json.hpp"
#include <fstream>

using json = nlohmann::json;

#define CreateComObject(clsid, iid, var) CoCreateInstance( clsid, NULL, CLSCTX_INPROC_SERVER, iid, (void **)&var);

STDAPI AMovieSetupRegisterServer(CLSID   clsServer, LPCWSTR szDescription, LPCWSTR szFileName, LPCWSTR szThreadingModel = L"Both", LPCWSTR szServerType = L"InprocServer32");
STDAPI AMovieSetupUnregisterServer(CLSID clsServer);



// {8fed61c9-598c-4f6e-bf3e-0706732cc264}
//DEFINE_GUID(CLSID_VirtualCam,
//            0x8fed61c9, 0x598c, 0x4f6e, 0xbf, 0x3e, 0x07, 0x06, 0x73, 0x2c, 0xc2, 0x64);
CLSID CLSID_VirtualCam;

WCHAR  name[100];

WCHAR memnamedata[100];
WCHAR memnameconfig[100];
WCHAR memnamelock[100];

struct RESOLUTION* resolution;
int resolutionsize;

struct RESOLUTION resolutionbuffer[10];


const AMOVIESETUP_MEDIATYPE AMSMediaTypesVCam =
{
	&MEDIATYPE_Video,
	&MEDIASUBTYPE_NULL
};

const AMOVIESETUP_PIN AMSPinVCam =
{
	L"Output",             // Pin string name
	FALSE,                 // Is it rendered
	TRUE,                  // Is it an output
	FALSE,                 // Can we have none
	FALSE,                 // Can we have many
	&CLSID_NULL,           // Connects to filter
	NULL,                  // Connects to pin
	1,                     // Number of types
	&AMSMediaTypesVCam      // Pin Media types
};

const AMOVIESETUP_FILTER AMSFilterVCam =
{
	&CLSID_VirtualCam,  // Filter CLSID
	name,     // String name
	MERIT_DO_NOT_USE,      // Filter merit
	1,                     // Number pins
	&AMSPinVCam             // Pin details
};

CFactoryTemplate g_Templates[] =
{
	{
		name,
		&CLSID_VirtualCam,
		CVCam::CreateInstance,
		NULL,
		&AMSFilterVCam
	},

};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

STDAPI RegisterFilters(BOOL bRegister)
{
	HRESULT hr = NOERROR;
	WCHAR achFileName[MAX_PATH];
	char achTemp[MAX_PATH];
	ASSERT(g_hInst != 0);

	if (0 == GetModuleFileNameA(g_hInst, achTemp, sizeof(achTemp)))
		return AmHresultFromWin32(GetLastError());

	MultiByteToWideChar(CP_ACP, 0L, achTemp, lstrlenA(achTemp) + 1,
		achFileName, NUMELMS(achFileName));

	hr = CoInitialize(0);
	if (bRegister)
	{
		hr = AMovieSetupRegisterServer(CLSID_VirtualCam, name, achFileName, L"Both", L"InprocServer32");
	}

	if (SUCCEEDED(hr))
	{
		IFilterMapper2* fm = 0;
		hr = CreateComObject(CLSID_FilterMapper2, IID_IFilterMapper2, fm);
		if (SUCCEEDED(hr))
		{
			if (bRegister)
			{
				IMoniker* pMoniker = 0;
				REGFILTER2 rf2;
				rf2.dwVersion = 1;
				rf2.dwMerit = MERIT_DO_NOT_USE;
				rf2.cPins = 1;
				rf2.rgPins = &AMSPinVCam;
				hr = fm->RegisterFilter(CLSID_VirtualCam, name, &pMoniker, &CLSID_VideoInputDeviceCategory, NULL, &rf2);
			}
			else
			{
				hr = fm->UnregisterFilter(&CLSID_VideoInputDeviceCategory, 0, CLSID_VirtualCam);
			}
		}

		// release interface
		//
		if (fm)
			fm->Release();
	}

	if (SUCCEEDED(hr) && !bRegister)
		hr = AMovieSetupUnregisterServer(CLSID_VirtualCam);


	CoFreeUnusedLibraries();
	CoUninitialize();
	return hr;
}

HINSTANCE Inst;

STDAPI DllRegisterServer()
{
	return RegisterFilters(TRUE);
}

STDAPI DllUnregisterServer()
{
	return RegisterFilters(FALSE);
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

__declspec(dllexport) BOOL APIENTRY DllMain(HANDLE hModule, DWORD  dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH) {
		Inst = (HINSTANCE)(hModule);

		char szFileName[MAX_PATH];
		GetModuleFileNameA(Inst, szFileName, MAX_PATH);
		PathRemoveExtensionA(szFileName);
		PathAddExtensionA(szFileName, ".json");

		std::ifstream i(szFileName);
		json config;
		i >> config;


		//{char str[1024]; wsprintfA(str, "[ALEX] filename %s\n", szFileName); OutputDebugStringA(str); }

		std::string tname = config["name"].get<std::string>();
		std::wstring wname = std::wstring(tname.begin(), tname.end());
		wcscpy_s(name, wname.c_str());


		std::string clsidstr = config["clsis"].get<std::string>();
		std::wstring clsidwstr = std::wstring(clsidstr.begin(), clsidstr.end());
		HRESULT t = CLSIDFromString(clsidwstr.c_str(), &CLSID_VirtualCam);

		
		std::string tmemnamedata = config["data"].get<std::string>();
		std::wstring wmemnamedata = std::wstring(tmemnamedata.begin(), tmemnamedata.end());
		wcscpy_s(memnamedata, wmemnamedata.c_str());




		std::string tmemnameconfig = config["config"].get<std::string>();
		std::wstring wmemnameconfig = std::wstring(tmemnameconfig.begin(), tmemnameconfig.end());
		wcscpy_s(memnameconfig, wmemnameconfig.c_str());


		std::string tmemnamelock = config["lock"].get<std::string>();
		std::wstring wmemnamelock = std::wstring(tmemnamelock.begin(), tmemnamelock.end());
		wcscpy_s(memnamelock, wmemnamelock.c_str());



		resolutionsize = (int)config["resolutions"].size();


		{char str[1024]; wsprintfA(str, "[ALEX] -----------------------------------------"); OutputDebugStringA(str); printf(str); }
		{char str[1024]; wsprintfA(str, "[ALEX] JSON %s\n", szFileName); OutputDebugStringA(str); printf(str); }
		{char str[1024]; wsprintfA(str, "[ALEX] name %S\n", name); OutputDebugStringA(str); printf(str); }
		{char str[1024]; wsprintfA(str, "[ALEX] clsis %S\n", clsidwstr.c_str()); OutputDebugStringA(str); printf(str); }
		{char str[1024]; wsprintfA(str, "[ALEX] memnamedata %S\n", memnamedata); OutputDebugStringA(str); printf(str); }
		{char str[1024]; wsprintfA(str, "[ALEX] memnameconfig %S\n", memnameconfig); OutputDebugStringA(str); printf(str); }
		{char str[1024]; wsprintfA(str, "[ALEX] memnamelock %S\n", memnamelock); OutputDebugStringA(str); printf(str); }
		{char str[1024]; wsprintfA(str, "[ALEX] size %d\n", resolutionsize); OutputDebugStringA(str); printf(str); }

		resolution = new struct RESOLUTION[resolutionsize]; 
		int c = 0;
		for (json::iterator it = config["resolutions"].begin(); it != config["resolutions"].end(); ++it) {
			resolution[c].x = (*it)["width"].get<int>();
			resolution[c].y = (*it)["height"].get<int>();
			resolution[c].avg = (*it)["time"].get<int>();
			c++;
			{char str[1024]; wsprintfA(str, "[ALEX] resolution %d %d %d\n", (*it)["width"].get<int>(), (*it)["height"].get<int>(), (*it)["time"].get<int>()); OutputDebugStringA(str); printf(str); printf(str); }
		}
	}
	else if (dwReason == DLL_PROCESS_DETACH) {
		delete resolution;
	}


	return  DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}



