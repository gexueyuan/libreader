CROSS = /home/gexueyuan/bin/toolchain-mips_34kc_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mips-openwrt-linux-
CC   = $(CROSS)gcc
CXX  = $(CROSS)g++
OBJCOPY = $(CROSS)objcopy

LOCAL_INCLUDE?=/home/gexueyuan/bin/toolchain-mips_34kc_gcc-4.8-linaro_uClibc-0.9.33.2/usr/include
LOCAL_LIB?=/home/gexueyuan/bin/toolchain-mips_34kc_gcc-4.8-linaro_uClibc-0.9.33.2/usr/lib
HOME_INCLUDE?=/home/gexueyuan/lua/src/
HOME_LIB?=$(HOME)/lib

USB_LIB?=../libusb-1.0.a
LUA_LIB?=../liblua.a
MCRYPTO_LIB?=$(LOCAL_LIB)/mcrypto.a
#export MCRYPTO_LIB=/home/pi/src/mcrypto/mcrypto.a

CFLAGS   = -Wall -fPIC -O2
#CFLAGS   = -Wall -g
LDFLAGS=

LIBNAME = ./libreader.so

OBJ	  := ./src/luareader.o \
	../luausb/compat.o \
	../luausb/enums.o \
	../luausb/structs.o \
	../luausb/usb.o

SCRIPTOBJ := ./reader_classes_usb_lua.o \
	./reader_lua.o \
	./reader_shell_lua.o
	
CRYPTOOBJ	  := ../luacrypto/src/lcrypto.o
	
INCLUDE =-I$(HOME_INCLUDE) -I$(LOCAL_INCLUDE)/libusb-1.0 -I$(HOME_INCLUDE)/libusb-1.0
LIBS =-ldl -lm -ludev -lpthread $(USB_LIB) $(LUA_LIB)

build: unix

clean:
	rm -f $(OBJ) $(CRYPTOOBJ) $(LIBNAME) core test

unix: $(OBJ)
	$(CC) -s -shared $(LDFLAGS) $^ $(SCRIPTOBJ) $(LIBS) -o $(LIBNAME) -Wl,--version-script=func_list.txt

crypto: CFLAGS+=-DLUAREADER_SUPPORT_CRYPTO=1

crypto:$(OBJ)	$(CRYPTOOBJ)
	$(CC) -s -shared $(LDFLAGS) $^ $(SCRIPTOBJ) $(LIBS) $(MCRYPTO_LIB) -o $(LIBNAME) -Wl,--version-script=func_list.txt

lua_i386:
	objcopy -I binary -O elf32-i386 -B i386 reader_classes_usb.lua reader_classes_usb_lua.o
	objcopy -I binary -O elf32-i386 -B i386 reader.lua reader_lua.o
	objcopy -I binary -O elf32-i386 -B i386 reader_shell.lua reader_shell_lua.o
	
lua_arm:
	objcopy -I binary -O elf32-littlearm -B arm reader_classes_usb.lua reader_classes_usb_lua.o
	objcopy -I binary -O elf32-littlearm -B arm reader.lua reader_lua.o
	objcopy -I binary -O elf32-littlearm -B arm reader_shell.lua reader_shell_lua.o
	
lua_mips:
	$(OBJCOPY) -I binary -O elf32-tradbigmips -B mips:isa32 reader_classes_usb.lua reader_classes_usb_lua.o
	$(OBJCOPY) -I binary -O elf32-tradbigmips -B mips:isa32 reader.lua reader_lua.o
	$(OBJCOPY) -I binary -O elf32-tradbigmips -B mips:isa32 reader_shell.lua reader_shell_lua.o
test:
	$(CC) test.c $(LIBNAME) -o test
changeapp:
	$(CC) changeapp.c $(LIBNAME) -o changeapp
	
$(OBJ) : %.o : %.c
	$(CC) $(CFLAGS) -Dluaopen_module=luaopen_usb -c $< -o $@ $(INCLUDE) 

$(CRYPTOOBJ) : %.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDE) 
