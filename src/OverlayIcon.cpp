// OverlayIcon.cpp : Implementation of DLL Exports.

#include "stdafx.h"
#include "resource.h"
#include "OverlayIcon.h"
#include <CommCtrl.h>
#include <SDKDDKVer.h>
#include <cstdio>
#include <windows.h>
#include <cstddef>
#include <Psapi.h>
#include <winternl.h>
#include "Patcher.h"
#include <commctrl.h>
#pragma comment(lib, "ntdll.lib")
#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "Comctl32.lib")

class COverlayIconModule : public CAtlDllModuleT< COverlayIconModule >
{
public :
	DECLARE_LIBID(LIBID_OverlayIconLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_OVERLAYICON, "{FA8EDCDD-EFA2-477B-B00A-7F28F02CD37E}")
};

COverlayIconModule _AtlModule;

FILE *image_list_log;
LPVOID patch_result;


/*
 * This is a function which is going to be called after calling patched function. 
 */
extern "C" __declspec(dllexport) BOOL WINAPI MySetOverlayImage(HIMAGELIST himl,
	int        iImage,
	int        iOverlay) {

	// log all calls
	const char *log_path = "C:\\Users\\akvinikym\\imagelist_log.txt";
	image_list_log = fopen(log_path, "a+");
	fprintf(image_list_log, "called patched function: ");
	fprintf(image_list_log, "image list %p, size: %d\n", himl, ImageList_GetImageCount(himl));
	fclose(image_list_log);

	return true;
}

// DLL Entry Point
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	hInstance;
	if (dwReason == DLL_PROCESS_ATTACH) {
		auto module = GetModuleHandle(TEXT("OverlayIcon.dll"));

		auto fun_address = GetProcAddress(
			module,
			"MySetOverlayImage"
		);

		patch_result = Patch(
			TEXT("comctl32.dll"),
			TEXT("ImageList_SetOverlayImage"),
			fun_address,
			Arch::x64
		);
		if (!patch_result)
		{
			MessageBox(NULL, "PATCH FAILED", "for some reason", MB_OK);
		}
		else {
			MessageBox(NULL, "PATCHED", "HELLO", MB_OK);
		}
	}
    return _AtlModule.DllMain(dwReason, lpReserved); 
}


// Used to determine whether the DLL can be unloaded by OLE
STDAPI DllCanUnloadNow(void)
{
    return _AtlModule.DllCanUnloadNow();
}


// Returns a class factory to create an object of the requested type
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _AtlModule.DllGetClassObject(rclsid, riid, ppv);
}


// DllRegisterServer - Adds entries to the system registry
STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    HRESULT hr = _AtlModule.DllRegisterServer();
	return hr;
}


// DllUnregisterServer - Removes entries from the system registry
STDAPI DllUnregisterServer(void)
{
	HRESULT hr = _AtlModule.DllUnregisterServer();
	return hr;
}
