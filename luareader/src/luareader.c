// luareader.c : 定义 DLL 应用程序的导出函数。
//
#include "stdafx.h"
#include <stdlib.h>
#include "luareader.h"

#ifdef WIN32
#include <lua/lua.hpp>
extern int luaopen_sysapi(lua_State *L);
extern LPCSTR GetCustomResource(int id, size_t * resource_length);
#else
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
int luaopen_usb(lua_State *L);
#endif
#if LUAREADER_SUPPORT_CRYPTO
int luaopen_crypto(lua_State *L);
#endif


// input: cmd, input, max_output_size
// ouput: errno, ouput
static int _luareader_callback(lua_State *L)
{
	// input
	const char * cmd = luaL_checkstring(L, 1);
	size_t input_size = 0;
	const char * input = luaL_optlstring(L, 2, "", &input_size);
	lua_Integer max_output_size = luaL_optinteger(L, 3, 0);

	unsigned char * output = NULL;
	int ret = -1;
	luareader_callback_function callback = NULL;

	// Lua执行后取得全局变量的值 
	lua_getglobal(L, "_luareader_callback_c"); 
	callback = (luareader_callback_function)lua_touserdata(L, -1);
	lua_pop(L, 1);  /* remove */

	if (callback != NULL)
	{
		if (max_output_size > 0)
		{
			output = (unsigned char *)malloc((size_t)max_output_size);
		}
		ret = callback(L, cmd, (unsigned char *)input, (int)input_size, output, (int)max_output_size);
	}

	// output
	if (ret >= 0)
	{
		lua_pushinteger(L, 0);
		lua_pushlstring(L, (const char *)output, ret);
	}
	else
	{
		lua_pushinteger(L, ret);
		lua_pushstring(L, "callback error!");
	}

	if (output != NULL)
	{
		free(output);
	}
	return 2;
}

static int _luareader_doresource(lua_State* L, int id, const char * name, const char * script)
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
		extern int _binary_reader_classes_usb_lua_start;
		extern int _binary_reader_classes_usb_lua_end;
		//extern int _binary_reader_classes_usb_lua_size;
		resource = (const char *)&_binary_reader_classes_usb_lua_start;
		resource_length = (int)(((const char *)&_binary_reader_classes_usb_lua_end) - resource);
	}
	else if (id == 2)
	{
		extern int _binary_reader_lua_start;
		extern int _binary_reader_lua_end;
		resource = (const char *)&_binary_reader_lua_start;
		resource_length = (int)(((const char *)&_binary_reader_lua_end) - resource);
	}
	else/* if (id == 3)*/
	{
		extern int _binary_reader_shell_lua_start;
		extern int _binary_reader_shell_lua_end;
		resource = (const char *)&_binary_reader_shell_lua_start;
		resource_length = (int)(((const char *)&_binary_reader_shell_lua_end) - resource);
	}
#endif
	if (memcmp(resource, "\xEF\xBB\xBF", 3) == 0) // utf-8
	{
		resource += 3;
		resource_length -= 3;
	}

	ret = luaL_loadbuffer(L, resource, resource_length, name) || lua_pcall(L, 0, LUA_MULTRET, 0); 
	if (ret == LUA_OK)
	{
		if (name)
		{
			lua_setglobal(L, name);
		}
		if (script)
		{
			luaL_dostring(L, script);
		}
	}
	return ret;
}

LUAREADER_API void * luareader_new(int flags, const char *script, luareader_callback_function callback)
{
	int ret = LUA_OK;
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);
	
	//在注册完所有的C函数之后，即可在Lua的代码块中使用这些已经注册的C函数了。
	if (callback != NULL)
	{
		// 压入轻量级userdata，一个static函数指针
		lua_pushlightuserdata(L, callback);
		lua_setglobal(L, "_luareader_callback_c"); 

		// 注册函数
		lua_register(L, "_luareader_callback", _luareader_callback);
	}
#ifdef WIN32
	luaL_requiref(L, "sysapi", luaopen_sysapi, 1); // lua_pop(L, 1);
#else
	luaL_requiref(L, "usb", luaopen_usb, 1); // lua_pop(L, 1);
#endif

#if LUAREADER_SUPPORT_CRYPTO
	luaL_requiref(L, "crypto", luaopen_crypto, 1); // lua_pop(L, 1);
#endif

#ifdef WIN32
	ret = _luareader_doresource(L, 1, "global_resource1", "package.loaded['reader_classes_win32']=global_resource1\nglobal_resource1=nil"); // lua_pop(L, 1);
#else
	ret = _luareader_doresource(L, 1, "global_resource1", "package.loaded['reader_classes_usb']=global_resource1\nglobal_resource1=nil");
#endif
	if (ret == LUA_OK)
	{
		ret = _luareader_doresource(L, 2, "global_resource2", "package.loaded['reader']=global_resource2\nglobal_resource2=nil"); // lua_pop(L, 1);
	}
	if (ret == LUA_OK)
	{
		ret = _luareader_doresource(L, 3, NULL, NULL);
	}

	if (script != NULL)
	{
		if (flags & 1)
		{
			ret = luaL_dofile(L, script);  // lua_pop(L, 1); // 加载运行脚本文件, 
		}
		else
		{
			ret = luaL_dostring(L, script);  // lua_pop(L, 1); // 加载运行脚本文件, 
		}
	}

	if (ret != LUA_OK)
	{
		if (callback != NULL)
		{
			const char* err = lua_tostring(L,-1); 
			callback(L, "log", (unsigned char*)err, strlen(err), NULL, 0);
		}
		lua_close(L);
		return NULL;
	}

	lua_settop(L, 0);
	return L;
}

LUAREADER_API int luareader_term(void *context)
{
	lua_State* L = (lua_State*)context;
	if (L == NULL)
	{
		return -1001;
	}

	lua_close(L);
	return 0;
}


LUAREADER_API int luareader_get_list(void *context, char * reader_names, int max_reader_names_size)
{
	return luareader_do_task(context, "get_list", NULL, 0, (unsigned char *)reader_names, max_reader_names_size);
}

LUAREADER_API int luareader_connect(void *context, const char * reader_name)
{
	return luareader_do_task(context, "connect", (unsigned char *)reader_name, (int)strlen(reader_name), NULL, 0);
}

static int _lua_pop_value(lua_State* L, unsigned char * output, int max_output_size)
{
	int ret = 0;
	if (lua_gettop(L) > 0)
	{
		if (lua_isstring(L,-1) && (output != NULL))
		{
			size_t len = 0;
			const char * ptr = luaL_checklstring(L, -1, &len);//从栈中取回返回值  

			memcpy(output, ptr, ((int)len > max_output_size)? (int)max_output_size:len);
			ret = (int)len;
		}
		lua_pop(L, 1);//清栈，由于当前只有一个返回值  
	}
	return ret;
}

LUAREADER_API int luareader_transmit(void *context, const unsigned char * apdu, int apdu_len, unsigned char * resp, int max_resp_size, int timeout)
{
	//return luareader_do_task(context, "transmit", apdu, apdu_len, resp, max_resp_size);
	int ret = 0;
	lua_State* L = (lua_State*)context;
	if (L == NULL)
	{
		return -1001; // 上下文错误
	}

	lua_getglobal(L, "transmit"); //查找lua_add函数,并压入栈底 

	lua_pushlstring(L, (const char*)apdu, apdu_len); //函数参数1  
	lua_pushinteger(L, timeout); //函数参数2  

	ret = lua_pcall(L, 2, LUA_MULTRET, 0);//调用lua_add函数，同时会对lua_add及两个参加进行出栈操作,并压入返回值  
	if (ret != LUA_OK)
	{
		return (ret > 0)? (-ret) : ret;
	}

	return _lua_pop_value(L, resp, max_resp_size);
}

LUAREADER_API int luareader_disconnect(void *context)
{
	return luareader_do_task(context, "disconnect", NULL, 0, NULL, 0);
}

LUAREADER_API int luareader_do_task(void *context, const char * tast_name, const unsigned char * input, int input_len, unsigned char * output, int max_output_size)
{
	int ret = 0;
	lua_State* L = (lua_State*)context;
	if (L == NULL)
	{
		return -1001; // 上下文错误
	}

	lua_getglobal(L, tast_name); //查找lua_add函数,并压入栈底 

	lua_pushlstring(L, (const char*)input, input_len); //函数参数1  

	ret = lua_pcall(L, 1, LUA_MULTRET, 0);//调用lua_add函数，同时会对lua_add及两个参加进行出栈操作,并压入返回值  
	if (ret != LUA_OK)
	{
		return (ret > 0)? (-ret) : ret;
	}

	return _lua_pop_value(L, output, max_output_size);
}


LUAREADER_API int luareader_do_string(void *context, const char * str, unsigned char * output, int max_output_size)
{
	int ret = 0;
	lua_State* L = (lua_State*)context;
	if (L == NULL)
	{
		return -1001; // 上下文错误
	}
	
	ret = luaL_dostring(L, str);
	if (ret != LUA_OK)
	{
		return (ret > 0)? (-ret) : ret;
	}
	  
	return _lua_pop_value(L, output, max_output_size);
}

LUAREADER_API int luareader_pop_value(void *context, char * value, int max_value_size)
{
	lua_State* L = (lua_State*)context;
	if (L == NULL)
	{
		return -1001; // 上下文错误
	}
	if (lua_gettop(L) <= 0)
	{
		return -1003; // 空栈
	}
	else
	{
		return _lua_pop_value(L, value, max_value_size);
	}	
}
