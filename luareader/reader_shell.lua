
local reader = require('reader')

function get_list()
	return table.concat(reader.list(), '\x00') .. '\x00\x00'
end

local reader_obj
function connect(reader_name)
	reader_obj = assert(reader.connect(reader_name))
	return ''
end

function transmit(apdu)
	return assert(reader_obj:transmit(apdu, 3000))
end

function disconnect()
	reader_obj:disconnect()
	return ''
end
