

local function reader_classes_usb_init()
	local usb = require('usb')
	local base = {_type='base'}
	local hid  = {_type='hid'}
	local scsi = {_type='scsi'}
	local pcsc = {_type='pcsc'}
		
	function chkret(fn, exp, ...)
		if exp then
			return exp, ...
		else
			local arg = { ... }
			error(string.format('%s fail! error=[%d][0x%s][%s]', fn, arg[2], string.format('%08X', arg[2]):sub(-8), arg[1]), 2)
		end
	end
	
	--usb context
	local usb_context = {}
	function usb_context.__gc(obj)
		if obj.ctx then
			usb.exit(obj.ctx)
			obj.ctx = nil
		end
	end
	setmetatable(usb_context, usb_context) 
	
	usb_context.ctx = chkret('usb.init', usb.init(1))

	function _getDevBusPortNumbers(dev)
		local busBum = dev:get_bus_number()
		local portNums = dev:get_port_numbers() or '' --byte array
		local portNums_desc = portNums:gsub('.', function (c) return string.format(".%d", string.byte(c)) end)
		return string.format("%d-", busBum) .. portNums_desc:sub(2)		
	end

	--[base] context, print,list,match,connect,disconnect
	base.context = usb_context.ctx	
	function base.print()
		local devList = usb_context.ctx:get_device_list()
		for k, dev in pairs(devList) do
			local dd = dev:get_device_descriptor()
			
			print(string.format("%d: VID=0x%04X, PID=0x%04X, BusBum-PortNumbers=%s", k, dd.idVendor, dd.idProduct, _getDevBusPortNumbers(dev)));

			local cd = dev:get_config_descriptor(0)
			print(string.format("cd: ConfigurationValue=%d, bNumInterfaces=%d", cd.bConfigurationValue, cd.bNumInterfaces))	
			for i,v in pairs(cd.interface) do
				for i2,v2 in ipairs(v.altsetting) do
					print(string.format("cd-interface: %d, altsetting[%d]-bInterfaceNumber=%d, bNumEndpoints=%d", i, i2, v2.bInterfaceNumber, v2.bNumEndpoints))
					for i3,v3 in pairs(v2.endpoint) do
						print(string.format("\tendpoint[%d]-bEndpointAddress=0x%02X, wMaxPacketSize=%d", i3, v3.bEndpointAddress, v3.wMaxPacketSize))
					end	
				end	
			end	
		end
	end

	function base.list(pidCVal)
		local result = {}
		for _, dev in pairs(usb_context.ctx:get_device_list()) do
			local dd = dev:get_device_descriptor()
			if (dd.idVendor == 0x1780) and (not pidCVal or ((dd.idProduct & 0x00F0) == pidCVal)) then
				table.insert(result, _getDevBusPortNumbers(dev))
			end
		end
		return result
	end	

	function base.match(obj, devPath)
		local devList = usb_context.ctx:get_device_list()
		for _, dev in pairs(devList) do		
			if devPath == _getDevBusPortNumbers(dev) then
				local dd = chkret('dev:get_device_descriptor', dev:get_device_descriptor())
				obj.interfaceNumber = 0
				obj.outputEndpoint = 0x04
				obj.inputEndpoint = 0x83
				obj.idVendor = dd.idVendor
				obj.idProduct = dd.idProduct
				
				local commType = nil
				if base.on_match then
					commType = base.on_match(obj)				
				end
				if not commType then
					commType = dd.idProduct & 0x00F0;
				end
				
				if commType == 0x0010 then --hid
					return hid
				elseif commType == 0x0000 then --scsi
					return scsi
				elseif commType == 0x0030 then --pcsc/ccid
					return pcsc
				else
					assert(nil, string.format('not support productID=0x%04X, error=[-1]', dd.idProduct))
				end
			end
		end
		return nil, 'no device!';
	end

	function base.connect(obj, devPath)
		local devList = usb_context.ctx:get_device_list()
		for _, dev in pairs(devList) do		
			if devPath == _getDevBusPortNumbers(dev) then
				obj.fd = chkret('dev:open', dev:open())
				if obj.fd:kernel_driver_active(obj.interfaceNumber) then
					chkret('detach_kernel_driver', obj.fd:detach_kernel_driver(obj.interfaceNumber))
				end
				assert(obj.fd:claim_interface(obj.interfaceNumber))
				return
			end
		end
		assert(nil, "not found device:" .. devPath .. ', error=[-1]');
	end
	
	function base.disconnect(obj)
		obj.fd:release_interface(obj.interfaceNumber)
		obj.fd:close()
		obj.fd = nil
	end	
	
	--[hid] list,match,connect,disconnect,flush,write,read
	function hid.list()
		return base.list(0x0010)
	end
	
	function hid.match(obj, devName)
		if base.match(obj, devPath) == hid then
			return hid
		else
			return nil, 'no hid device!';
		end
	end
	
	hid.connect = base.connect
	hid.disconnect = base.disconnect
	
	function hid.flush(obj)
	end	
		
	function hid.write(obj, data, timeout)
		local result = chkret('[hid]bulk_transfer', obj.fd:bulk_transfer(obj.outputEndpoint, data, timeout))
		assert(result==#data, '[hid]bulk_transfer send data result:' .. result .. ', error=[-1]')
	end

	function hid.read(obj, length, timeout)	
		local result = chkret('[hid]bulk_transfer', obj.fd:bulk_transfer(obj.inputEndpoint, length, timeout))
		return result
	end

	
	--[scsi] list,connect,disconnect,write,read
	function scsi.list()
		return base.list(0x0000)
	end

	function scsi.match(obj, devName)
		if base.match(obj, devPath) == scsi then
			return scsi
		else
			return nil, 'no scsi device!';
		end
	end
	
	scsi.connect = base.connect
	scsi.disconnect = base.disconnect
	
	function scsi.write(obj, data, timeout)		
		local result = chkret('[scsi]bulk_transfer', obj.fd:bulk_transfer(obj.outputEndpoint, '\x55\x53\x42\x43' .. '\x12\x34\x56\x78' .. string.pack("<I4", #data) .. '\x00' .. '\x00' .. '\x06\xF1' .. string.rep('\x00',15), timeout))
		assert(result==31, '[scsi]CBW result:' .. result .. ', error=[-1]')

		local result = chkret('[scsi]bulk_transfer', obj.fd:bulk_transfer(obj.outputEndpoint, data, timeout))
		assert(result==#data, '[scsi]CBW result:' .. result .. ', error=[-1]')
		
		-- WSignature(4) + Tag(4-Rand) + DataResidue(4) + Status(1)
		local result = chkret('[scsi]bulk_transfer', obj.fd:bulk_transfer(obj.inputEndpoint, 13, timeout))
		assert(result:sub(1,4)=='\x55\x53\x42\x53', '[scsi]CSW signature error!, error=[-1]')
		assert(result:sub(13,13)=='\x00', '[scsi]CSW status error!, error=[-1]')
	end

	function scsi.read(obj, length, timeout)
		local result = chkret('[scsi]bulk_transfer', obj.fd:bulk_transfer(obj.outputEndpoint, '\x55\x53\x42\x43' .. '\x12\x34\x56\x78' .. string.pack("<I4", 4096) .. '\x80' .. '\x00' .. '\x06\xF2' .. string.rep('\x00',15), timeout))
		assert(result==31, '[scsi]CBW result:' .. result .. ', error=[-1]')
		
		local resp = chkret('[scsi]bulk_transfer', obj.fd:bulk_transfer(obj.inputEndpoint, length, timeout))

		-- WSignature(4) + Tag(4-Rand) + DataResidue(4) + Status(1)
		local result = chkret('[scsi]bulk_transfer', obj.fd:bulk_transfer(obj.inputEndpoint, 13, timeout))
		assert(result:sub(1,4)=='\x55\x53\x42\x53', '[scsi]CSW signature error!, error=[-1]')
		assert(result:sub(13,13)=='\x00', '[scsi]CSW status error!, error=[-1]')

		local validDataLen = (resp:byte(1, 1) << 8) + resp:byte(2, 2)
		assert((2+validDataLen)==#resp, '[scsi] data length:' .. validDataLen .. ', error=[-1]')

		local validDataLen = string.unpack('>I2', resp) --be.bin2Int(r:sub(1,2))
		if ((2+validDataLen) > #resp) then
			return scsi.read(obj, validDataLen, timeout)
		end
		return resp:sub(3,2+validDataLen)
	end
	
	--[scsi] testReady
	function scsi.testReady(obj, timeout)
		-- WSignature(4) + Tag(4-Rand) + DataTransferLength(4) + Flags(1-00/80) + LUN(1-00) + Length(1) + CB(16)
		local result = chkret('[scsi]bulk_transfer', obj.fd:bulk_transfer(obj.outputEndpoint, '\x55\x53\x42\x43' .. '\x12\x34\x56\x78' .. '\x00\x00\x00\x00' .. '\x00' .. '\x00' .. '\x06' .. string.rep('\x00',16), timeout))
		assert(result==31, '[scsi]CBW result:' .. result .. ', error=[-1]')

		-- WSignature(4) + Tag(4-Rand) + DataResidue(4) + Status(1)
		local result = chkret('bulk_transfer', obj.fd:bulk_transfer(obj.inputEndpoint, 13, timeout))
		assert(result:sub(1,4)=='\x55\x53\x42\x53', '[scsi]CSW signature error!, error=[-1]')
		assert(result:sub(13,13)=='\x00', '[scsi]CSW status error!, error=[-1]')
	end

	
	--[pcsc] list,connect,disconnect,write,read
	function pcsc.list()
		return base.list(0x0030)
	end

	function pcsc.match(obj, devName)
		if base.match(obj, devPath) == pcsc then
			return pcsc
		else
			return nil, 'no pcsc device!';
		end
	end
	
	pcsc.connect = base.connect
	pcsc.disconnect = base.disconnect
	
	function pcsc._write(obj, data, messageType, timeout)
		if (not obj.sequence) or (obj.sequence == 0xFF) then
			obj.sequence = 0
		end
		obj.sequence = obj.sequence + 1

		--messageType,length,slot,sequence,reserved/vcc,levelParameter
		--vcc 0x00-auto; 0x01-5V; 0x02-3V; 0x03-1.8V
		local reserved = (messageType == 0x62) and 0x01 or 0x00
		local packet = string.pack('B<i4BBB<i2', messageType, #data, 0x00, obj.sequence, reserved, 0x0000) .. data
		
		local result = chkret('[pcsc]bulk_transfer', obj.fd:bulk_transfer(obj.outputEndpoint, packet, timeout))
		assert(result==#packet, '[pcsc]bulk_transfer result:' .. result .. ', error=[-1]')		
	end
	function pcsc._read(obj, dlength, timeout)		
		local resp = chkret('[pcsc]bulk_transfer', obj.fd:bulk_transfer(obj.inputEndpoint, 10+dlength, timeout))

		local messageType,length,slot,sequence,status,err,chainParameter = string.unpack('B<i4BBBBB', resp)
		assert(sequence == obj.sequence, '[pcsc] sequence: ' .. sequence, 'error=[-1]')
		assert((10+length) <= #resp, '[pcsc] data length: ' .. length, 'error=[-1]')
		return status, resp:sub(11, 10+length)
	end
		
	function pcsc.write(obj, data, timeout)
		if (#data <= 2) and (data:byte(1,1) == 0xFF) then
			pcsc._write(obj, '', 0x63, timeout) --CCID_POWEROFF
			pcsc._read(obj, 0, timeout);
			pcsc._write(obj, '', 0x62, timeout) -- CCID_POWERON
		else
			pcsc._write(obj, data, 0x6F, timeout)
		end
	end
	function pcsc.read(obj, length, timeout)
		while(true) do
			local status,resp = pcsc._read(obj, length, timeout)
			if status ~= 2 then --Procedure byte
				return resp
			end
		end
	end
	

	return {base, hid, scsi, pcsc}
end

--[[
function test(classes)
	classes[1].print()
	print(table.concat(classes[1].list(), '\n'))
	local obj = {}
	print(classes[1].match(obj, '1-1.3')._type)
	
	classes[1].connect(obj, '1-1.3')
	classes[1].disconnect(obj)
end
function test2(classes)
	function callback(bus, ports, event)
		local devPath = bus .. '-' .. (ports:gsub(".", function(c) return '.'..c:byte(1,1) end)):sub(2)
		print(devPath)
		print(event)
	end
	local ctx = classes[1].context
	local handle = ctx:hotplug_register_callback(callback)
	ctx:handle_events()
	ctx:handle_events()
	ctx:hotplug_deregister_callback(handle)
end
test(reader_classes_usb_init())
--test2(reader_classes_usb_init())
--]]
return reader_classes_usb_init()
