// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#ifdef WIN32
#pragma once

#ifndef WINVER                          // 指定要求的最低平台是 Windows XP。
#define WINVER 0x0500           // 将此值更改为相应的值，以适用于 Windows 的其他版本。
#endif

#ifndef _WIN32_WINNT            // 指定要求的最低平台是 Windows XP。
#define _WIN32_WINNT 0x0500     // 将此值更改为相应的值，以适用于 Windows 的其他版本。
#endif

#ifndef _WIN32_WINDOWS          // 指定要求的最低平台是 Windows 98。
#define _WIN32_WINDOWS 0x0410 // 将此值更改为适当的值，以适用于 Windows Me 或更高版本。
#endif

#ifndef _WIN32_IE                       // 指定要求的最低平台是 Internet Explorer 6.0。
#define _WIN32_IE 0x0600        // 将此值更改为相应的值，以适用于 IE 的其他版本。
#endif

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头中排除极少使用的资料
// Windows 头文件:
#include <windows.h>

#else
#include <stdio.h>
#include <string.h>
#endif

// TODO: 在此处引用程序需要的其他头文件
#ifndef LUAREADER_SUPPORT_CRYPTO
#define LUAREADER_SUPPORT_CRYPTO 0
#endif
#define LUACRYPTO_API


