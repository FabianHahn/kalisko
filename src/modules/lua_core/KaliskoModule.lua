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

KaliskoModule = {}

function KaliskoModule:new(name)
	if name == nil then
		return nil
	end
	
	instance = {}
	self.__index = self
	setmetatable(instance, self)
	instance.name = name
	return instance
end

function KaliskoModule:getName()
	return self.name
end

function KaliskoModule:isLoaded()
	xcall = invokeXCall('xcall = { function = isModuleLoaded }; module = ' .. self.name)
	xcs = parseStore(xcall)
	return xcs.loaded > 0
end

function KaliskoModule:getAuthor()
	xcall = invokeXCall('xcall = { function = getModuleAuthor }; module = ' .. self.name)
	xcs = parseStore(xcall)
	return xcs.author
end

function KaliskoModule:getDescription()
	xcall = invokeXCall('xcall = { function = getModuleDescription }; module = ' .. self.name)
	xcs = parseStore(xcall)
	return xcs.description
end

function KaliskoModule:getVersion()
	xcall = invokeXCall('xcall = { function = getModuleVersion }; module = ' .. self.name)
	xcs = parseStore(xcall)
	
	if xcs.version ~= nil then
		return xcs.version.string
	else
		return nil
	end
end

function KaliskoModule:getBcVersion()
	xcall = invokeXCall('xcall = { function = getModuleBcVersion }; module = ' .. self.name)
	xcs = parseStore(xcall)
	
	if xcs.bcversion ~= nil then
		return xcs.bcversion.string
	else
		return nil
	end
end

function KaliskoModule:getReferenceCount()
	xcall = invokeXCall('xcall = { function = getModuleReferenceCount }; module = ' .. self.name)
	xcs = parseStore(xcall)
	return xcs.reference_count
end

function KaliskoModule:request()
	xcall = invokeXCall('xcall = { function = requestModule }; module = ' .. self.name)
	xcs = parseStore(xcall)
	return xcs.success > 0
end

function KaliskoModule:revoke()
	xcall = invokeXCall('xcall = { function = revokeModule }; module = ' .. self.name)
	xcs = parseStore(xcall)
	return xcs.success > 0
end
