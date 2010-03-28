--[[
Copyright (c) 2010, Kalisko Project Leaders
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
      in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
--]]

KaliskoLog = {}

function KaliskoLog:debug(message)
	xcall = invokeXCall('xcall = { function = logDebug }; message = "' .. message .. '"')
	xcs = parseStore(xcall)
	return xcs.success > 0
end

function KaliskoLog:info(message)
	xcall = invokeXCall('xcall = { function = logInfo }; message = "' .. message .. '"')
	xcs = parseStore(xcall)
	return xcs.success > 0
end

function KaliskoLog:warning(message)
	xcall = invokeXCall('xcall = { function = logWarning }; message = "' .. message .. '"')
	xcs = parseStore(xcall)
	return xcs.success > 0
end

function KaliskoLog:error(message)
	xcall = invokeXCall('xcall = { function = logError }; message = "' .. message .. '"')
	xcs = parseStore(xcall)
	return xcs.success > 0
end
