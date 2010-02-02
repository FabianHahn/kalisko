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
