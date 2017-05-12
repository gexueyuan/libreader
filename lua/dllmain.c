// dllmain.c : 定义 DLL 应用程序的入口点。


#ifdef WIN32
#define WIN32_LEAN_AND_MEAN             // 从 Windows 头中排除极少使用的资料
// Windows 头文件:
#include <windows.h>
#include "resource.h"

#define LUA_LIB
#include <lua/lua.hpp>

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

LPCSTR GetCustomResource(int id, size_t * resource_length)
{
    //定位我们的自定义资源，这里因为我们是从本模块定位资源，所以将句柄简单地置为NULL即可
    HRSRC hRsrc = FindResource(_hModule, MAKEINTRESOURCE(IDR_LUASCRIPT1-1+id), TEXT("LUASCRIPT"));
	if (hRsrc != NULL)
	{
		//获取资源的大小
		if (resource_length != NULL)
		{
			*resource_length = SizeofResource(_hModule, hRsrc);
		}

		{
			//加载资源
			HGLOBAL hGlobal = LoadResource(_hModule, hRsrc); 

			//锁定资源
			LPVOID pBuffer = LockResource(hGlobal); 
			return (LPCSTR)pBuffer;
		}
	}
	return NULL;
}
#else
#include <string.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#endif


#define LUA_A_R_RES_JSON						1
#define LUA_A_R_RES_logging						2
#define LUA_A_R_RES_reader_classes_win32		3
#define LUA_A_R_RES_reader_classes_usb			4
#define LUA_A_R_RES_reader						5
#define LUA_A_R_RES_reader_protocol_rsaencrypt	6
#define LUA_A_R_RES_reader_protocol_hid_fri		7
static int _lua_a_r_doresource(lua_State* L, int id, const char * name)
{
	int ret = -1;
#ifdef WIN32
	size_t resource_length = 0;
	LPCSTR resource = GetCustomResource(id, &resource_length);

	if (resource == NULL)
	{
		return ret;
	}
#else
	size_t resource_length = 0;
	const char * resource = NULL;
	if (id == 1)
	{
		extern int _binary_JSON_lua_start;
		extern int _binary_JSON_lua_end;
		resource = (const char *)&_binary_JSON_lua_start;
		resource_length = (int)(((const char *)&_binary_JSON_lua_end) - resource);
	}
	else if (id == 2)
	{
		extern int _binary_logging_lua_start;
		extern int _binary_logging_lua_end;
		resource = (const char *)&_binary_logging_lua_start;
		resource_length = (int)(((const char *)&_binary_logging_lua_end) - resource);
	}
	else if (id == 3)
	{
		return ret;
	}
	else if (id == 4)
	{
		extern int _binary_reader_classes_usb_lua_start;
		extern int _binary_reader_classes_usb_lua_end;
		resource = (const char *)&_binary_reader_classes_usb_lua_start;
		resource_length = (int)(((const char *)&_binary_reader_classes_usb_lua_end) - resource);
	}
	else if (id == 5)
	{
		extern int _binary_reader_lua_start;
		extern int _binary_reader_lua_end;
		resource = (const char *)&_binary_reader_lua_start;
		resource_length = (int)(((const char *)&_binary_reader_lua_end) - resource);
	}
	else if (id == 6)
	{
		extern int _binary_reader_protocol_rsaencrypt_lua_start;
		extern int _binary_reader_protocol_rsaencrypt_lua_end;
		resource = (const char *)&_binary_reader_protocol_rsaencrypt_lua_start;
		resource_length = (int)(((const char *)&_binary_reader_protocol_rsaencrypt_lua_end) - resource);
	}
	else/* if (id == 7)*/
	{
		extern int _binary_reader_protocol_hid_fri_lua_start;
		extern int _binary_reader_protocol_hid_fri_lua_end;
		resource = (const char *)&_binary_reader_protocol_hid_fri_lua_start;
		resource_length = (int)(((const char *)&_binary_reader_protocol_hid_fri_lua_end) - resource);
	}
#endif
	if (memcmp(resource, "\xEF\xBB\xBF", 3) == 0) // utf-8
	{
		resource += 3;
		resource_length -= 3;
	}

	ret = luaL_loadbuffer(L, resource, resource_length, name) || lua_pcall(L, 0, LUA_MULTRET, 0);  // pop
	if (ret == LUA_OK)
	{
		if (name)
		{
			luaL_getsubtable(L, LUA_REGISTRYINDEX, "_LOADED"); // pop
		    lua_setfield(L, -2, name);  /* _LOADED[modname] = module */
		}
		return 1;
	}
	else
	{
		return -ret;
	}
}

int lua_a_r_preload(lua_State *L, const char *name)
{
	if (strcmp(name, "crypto") == 0)
	{
		extern int luaopen_crypto(lua_State *L);
		luaL_requiref(L, "crypto", luaopen_crypto, 1); 
		return 1;
	}
#ifdef WIN32
	else if (strcmp(name, "sysapi") == 0)
	{
		extern int luaopen_sysapi(lua_State *L);
		luaL_requiref(L, "sysapi", luaopen_sysapi, 1); 
		return 1;
	}
#else
	else if (strcmp(name, "usb") == 0)
	{
		extern int luaopen_usb(lua_State *L);
		luaL_requiref(L, "usb", luaopen_usb, 1); 
		return 1;
	}
#endif
	else if (strcmp(name, "JSON") == 0)
	{
		return _lua_a_r_doresource(L, LUA_A_R_RES_JSON, name);
	}
	else if (strcmp(name, "logging") == 0)
	{
		return _lua_a_r_doresource(L, LUA_A_R_RES_logging, name);
	}
	else if (strcmp(name, "reader_classes_win32") == 0)
	{
		return _lua_a_r_doresource(L, LUA_A_R_RES_reader_classes_win32, name);
	}
	else if (strcmp(name, "reader_classes_usb") == 0)
	{
		return _lua_a_r_doresource(L, LUA_A_R_RES_reader_classes_usb, name);
	}
	else if (strcmp(name, "reader") == 0)
	{
		return _lua_a_r_doresource(L, LUA_A_R_RES_reader, name);
	}
	else if (strcmp(name, "reader_protocol_rsaencrypt") == 0)
	{
		return _lua_a_r_doresource(L, LUA_A_R_RES_reader_protocol_rsaencrypt, name);
	}
	else if (strcmp(name, "reader_protocol_hid_fri") == 0)
	{
		return _lua_a_r_doresource(L, LUA_A_R_RES_reader_protocol_hid_fri, name);
	}
	else
	{
		return 0;
	}
}
