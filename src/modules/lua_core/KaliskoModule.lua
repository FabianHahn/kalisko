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
	return isModuleLoaded({module = self.name}).loaded ~= 0
end

function KaliskoModule:getAuthor()
	return getModuleAuthor({module = self.name}).author
end

function KaliskoModule:getDescription()
	return getModuleDescription({module = self.name}).description
end

function KaliskoModule:getVersion()
	return getModuleVersion({module = self.name}).version.string
end

function KaliskoModule:getBcVersion()
	return getModuleBcVersion({module = self.name}).bcversion.string
end

function KaliskoModule:getReferenceCount()
	return getModuleReferenceCount({module = self.name}).reference_count
end

function KaliskoModule:getDependencies()
	ret = {}

	for i, modname in ipairs(getModuleDependencies({module = self.name}).modules) do
		table.insert(ret, KaliskoModule:new(modname))
	end
	
	return ret
end

function KaliskoModule:getReverseDependencies()
	ret = {}

	for i, modname in ipairs(getModuleReverseDependencies({module = self.name}).modules) do
		table.insert(ret, KaliskoModule:new(modname))
	end
	
	return ret
end

function KaliskoModule:request()
	return requestModule({module = self.name}).success ~= 0
end

function KaliskoModule:revoke()
	revokeModule({module = self.name})
end

function KaliskoModule:forceUnload()
	forceUnloadModule({module = self.name})
end

function KaliskoModule:forceReload()
	forceReloadModule({module = self.name})
end

function KaliskoModule:getActiveModules() -- declared as method for convenience, doesn't really need a self
	ret = {}
	
	for i, modname in ipairs(getActiveModules({module = self.name}).modules) do
		table.insert(ret, KaliskoModule:new(modname))
	end
	
	return ret
end
	
