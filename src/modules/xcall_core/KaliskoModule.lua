function KaliskoModule:new(name)
	instance = {}
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

	