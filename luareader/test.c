#include "./src/stdafx.h"
#include "./src/luareader.h"

#ifndef WIN32
#define _DEBUG
#endif

#ifdef _DEBUG
static int luareader_callback(void *context, const char * command, const unsigned char * input, int input_len, unsigned char * output, int max_output_size)
{
	//if (strcmp(command, "log") == 0)
	{
		puts((char*)input);
	}
	return 0;
}

#if defined(WIN32) && defined(_USRDLL)
__declspec(dllexport) int RunTestW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow)
#else
int main()
#endif
{
	void * context = luareader_new(1, "./reader_shell.lua", luareader_callback);
	//void * context = luareader_new(0, NULL, luareader_callback);
	
	unsigned char output[1024] = {0};
	int ret;
	
	ret = luareader_do_task(context, "test1", NULL, 0, output, sizeof(output));
	printf("luareader_do_task(%p)=%d\n", context, ret);
	ret = luareader_pop_value(context, (char *)output, sizeof(output));

	luareader_do_string(context, "local ctx=require('reader_classes_usb')[1].context\n ctx:hotplug_register_callback(function(bus, ports, event) _luareader_callback('event', bus .. '-' .. ports:encode()) end)\n ctx:handle_events()", NULL, 0);
	ret = luareader_pop_value(context, (char *)output, sizeof(output));

	ret = luareader_get_list(context, (char *)output, sizeof(output));
	printf("luareader_get_list(%p)=%d(%s)\n", context, ret, output);

	ret = luareader_connect(context, (char *)output);
	printf("luareader_connect(%p)=%d\n", context, ret);

	ret = luareader_transmit(context, (unsigned char*)"\x00\x84\x00\x00\x08", 5, output, sizeof(output));
	printf("luareader_transmit(%p)=%d\n", context, ret);
	
	ret = luareader_disconnect(context);

	memset(output, 0, sizeof(output));
	ret = luareader_pop_value(context, (char *)output, sizeof(output));
	printf("luareader_pop_value(%p)=%d(%s)\n", context, ret, output);

	luareader_term(context);
	return ret;
}
#endif
