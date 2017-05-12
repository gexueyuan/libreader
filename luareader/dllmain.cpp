// dllmain.cpp : ���� DLL Ӧ�ó������ڵ㡣
#include "stdafx.h"
#include "luareader.h"
#include "resource.h"


//�Զ����붯̬��
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
    //��λ���ǵ��Զ�����Դ��������Ϊ�����Ǵӱ�ģ�鶨λ��Դ�����Խ�����򵥵���ΪNULL����
    HRSRC hRsrc = FindResource(_hModule, MAKEINTRESOURCE(IDR_LUASCRIPT1+id-1), TEXT("LUASCRIPT"));
	if (hRsrc != NULL)
	{
		//��ȡ��Դ�Ĵ�С
		if (resource_length != NULL)
		{
			*resource_length = SizeofResource(_hModule, hRsrc);
		}

	    //������Դ
		HGLOBAL hGlobal = LoadResource(_hModule, hRsrc); 

		//������Դ
		LPVOID pBuffer = LockResource(hGlobal); 
		return (LPCSTR)pBuffer;
	}
	return NULL;
}

#include "../luasysapi/win32/sysapi.cpp"
#if LUAREADER_SUPPORT_CRYPTO
#include "../luacrypto/src/lcrypto.c"
#endif
