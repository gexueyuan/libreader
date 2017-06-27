


--[reader_protocol_hid] transmit
local reader_protocol_hid = {}
function reader_protocol_hid.transmit(obj, apdu, timeout)
	local subprot = obj.subprot or obj
	timeout = timeout or obj.timeout
	
	subprot:flush()
	
	function send_packet(packet, timeout)
		local totalLen = #packet
		local packageID = 0	
		for i=1, #packet, 60 do
			local currLen = totalLen + 1 - i
			local data
			if currLen <= 60 then
				data = packet:sub(i, totalLen) .. string.rep('\x00',60-currLen)
				totalLen = totalLen | 0x8000; --last packet
			else
				currLen = 60
				data = packet:sub(i, i+59)
			end
			subprot:write(string.pack(">I2BB", totalLen, packageID, currLen) .. data, timeout)
			packageID = packageID + 1
		end
	end
	
	function recv_packet(timeout)
		local resp = ''
		while true do
			local result = subprot:read(64, timeout)

			local totalLen, packageID, data = string.unpack(">I2Bs1", result)

			if (data ~= '\x60') then --过程字节
				resp = resp .. data
				if (totalLen & 0x8000) ~= 0 then
					break;
				end
			end
		end
		return resp
	end
	
	send_packet(apdu, timeout)
	return recv_packet(timeout)
end
	
--[reader_protocol_default] transmit, execute
local reader_protocol_default = {}
function reader_protocol_default.transmit(obj, apdu, timeout)
	local subprot = obj.subprot or obj
	timeout = timeout or obj.timeout

	subprot:write(apdu, timeout)
	return subprot:read(4096, timeout)
end

--内部处理SW，cond={SW='\x90\x00', getRespondApduHead='\x00\xC0\x00\x00'}
function reader_protocol_default.execute(obj, apdu, cond, timeout)
	local wantSW = (cond and cond.SW) or '\x90\x00'
	local resp = obj:transmit(apdu, timeout)
	local sw = resp:sub(-2)

	obj.sw = sw

	function findWantSW(wantSW, realSW)
		local realSW_num = string.unpack('>i2', realSW)
		
		function findWantSW_single(wantSW, realSW, realSW_num)
			if (type(wantSW) == 'string') then
				if (#wantSW == 0) then
					return 0
				elseif (#wantSW == #realSW) then
					return (wantSW == realSW) and 0 or nil
				else
					return (wantSW == realSW:sub(1,#wantSW)) and 0 or nil
				end
			else
				return (realSW_num == wantSW) and 0 or nil
			end
		end

		function findWantSW_table(wantSWList, realSW, realSW_num)
			for _,s in pairs(wantSWList) do
				local findResult = findWantSW_single(s, realSW, realSW_num)
				if findResult then
					return 1
				end
			end
			return nil
		end
		
		local findWantSW_fn = (type(wantSW) == 'table') and findWantSW_table or findWantSW_single
		local findResult = findWantSW_fn(wantSW, realSW, realSW_num)
		if findResult then
			return findResult,(realSW_num >> 8)
		else
			local realSW_s = realSW:encode()
			return nil,'Non expectation SW:' .. realSW_s .. string.format(', error=[][0xE0E0%s]', realSW_s);
		end
	end

	local findResult,sw1 = assert(findWantSW(wantSW, sw))
	if (findResult == 1) then
		if (sw1 == 0x61) and findWantSW(wantSW, '\x90\x00') then
			local totalResult = resp:sub(1,#resp-2)
			while sw:byte(1,1) == 0x61 do
				apdu = ((cond and cond.getRespondApduHead) or '\x00\xC0\x00\x00') .. sw:sub(2,2)
				resp = obj:transmit(apdu, timeout)
				sw = resp:sub(-2)
				assert((sw == '\x90\x00') or (sw:byte(1,1) == 0x61), 'Non expectation SW:'..sw:encode())
				totalResult = totalResult .. resp:sub(1,#resp-2)
				break;
			end
			return totalResult
		elseif (sw1 == 0x6C) and findWantSW(wantSW, '\x90\x00') then
			resp = obj:transmit(apdu:sub(1,4) .. sw:sub(2,2), timeout)
			sw = resp:sub(-2)
			assert(sw == '\x90\x00', 'Non expectation SW:'..sw:encode())
		end
	end

	return resp:sub(1,#resp-2)
end		
	
--[reader] __gc
local reader = {}
local function get_default_classes()
	local env_os = os.getenv('OS')
	if env_os and env_os:find('Windows') then --window
		return require('reader_classes_win32')
	else
		return require('reader_classes_usb')
	end
end

function reader.__gc(obj)
	if obj.fd then
		obj:disconnect(obj)
	end
end

--[all class] _classes, __index, __gc, transmit, execute
function reader.set_classes(classes)
	reader._classes = classes
	for _,cls in pairs(classes) do
		cls.__index = cls
		cls.__gc = reader.__gc
		if not cls.transmit then
			cls.transmit = ((cls._type == 'hid') and reader_protocol_hid.transmit) or reader_protocol_default.transmit
		end
		cls.execute = reader_protocol_default.execute
	end
end
reader.set_classes(get_default_classes())

--[reader] print,list,disconnect,transmit
function reader.print()
	reader._classes[1].print()
end

function reader.list(devType, devNameFilter)
	local function filterArray(ar, filter)
		local result = {}
		for _,v in ipairs(ar) do
			if v:find(filter) then
				table.insert(result, v)
			end
		end
		return result;
	end
	
	local devList
	if devType then --指定设备类型
		for _,cls in pairs(reader._classes) do
			if cls._type == devType then
				devList = cls.list()
				break
			end
		end
	else
		devList = reader._classes[1].list()
	end
	
	return (devNameFilter and filterArray(devList, devNameFilter)) or devList
end


--return [object] timeout, fd, [idProduct]	
function reader.connect(devName, options)
	local obj = {timeout=30000}

	for _,cls in pairs(reader._classes) do
		local rcls = cls.match(obj, devName)
		if rcls then
			setmetatable(obj, rcls)
			break;
		end
	end

	obj:connect(devName)
	if options and options.protocols then
		return obj,reader.create_protocol(obj, options.protocols)
	else
		return obj,obj;
	end
end

--options: {name='rsaencrypt', next={name='hexstring'}}
function reader.create_protocol(obj, options)		
	function _create(opt)
		if (opt and opt.name) then
			local protocol = require('reader_protocol_' .. opt.name)
			local nobj = protocol.new(_create(opt.subopt));			
			for k,v in pairs(opt) do
				if k~='name' and k~='subopt' then
					if (v:sub(1,1) == '@') then --string
						nobj[k] = v:sub(2);
					else
						nobj[k] = v:decode()
					end
				end
			end
			return nobj;
		else
			return obj
		end
	end

	return _create(options)
end
reader.createProtocol = reader.create_protocol

	
--[[ test
local function test()
	reader.print()
	local devNameList=reader.list()	
	local obj = reader.connect(arg[1] or devNameList[1])
	print(obj:execute(('FF'):decode()):encode())
	print(obj:execute(('00A4000002DF20'):decode()):encode())
	print(obj:execute(('00A4000002A314'):decode()):encode())
	print(obj:execute(('00B00000FF'):decode()):encode())
	
	local protocol = reader.create_protocol(obj, {name='hexstring', subopt={name='rsaencrypt'}})
	print(protocol:execute('0084000008'))

	--obj = nil --collectgarbage("collect")
	obj:disconnect()
end 
test() --]]

return reader
