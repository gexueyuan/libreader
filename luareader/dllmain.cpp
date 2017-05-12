// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"
#include "luareader.h"
#include "resource.h"


//自动载入动态库
#ifdef _M_IX86
#pragma comment(lib, "libluas.lib")
#if LUAREADER_SUPPORT_CRYPTO
#pragma comment(lib, "mcrypto.lib")
#endif
#else // _M_X64
#endif
#pragma message("Automatically linking with libluas.lib")


static HMODULE _hModule = NULL;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		_hModule = hModule;
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}


extern "C" 
LPCSTR GetCustomResource(int id, size_t * resource_length)
{
    //定位我们的自定义资源，这里因为我们是从本模块定位资源，所以将句柄简单地置为NULL即可
    HRSRC hRsrc = FindResource(_hModule, MAKEINTRESOURCE(IDR_LUASCRIPT1+id-1), TEXT("LUASCRIPT"));
	if (hRsrc != NULL)
	{
		//获取资源的大小
		if (resource_length != NULL)
		{
			*resource_length = SizeofResource(_hModule, hRsrc);
		}

	    //加载资源
		HGLOBAL hGlobal = LoadResource(_hModule, hRsrc); 

		//锁定资源
		LPVOID pBuffer = LockResource(hGlobal); 
		return (LPCSTR)pBuffer;
	}
	return NULL;
}

#include "../luasysapi/win32/sysapi.cpp"
#if LUAREADER_SUPPORT_CRYPTO
#include "../luacrypto/src/lcrypto.c"
#endif
