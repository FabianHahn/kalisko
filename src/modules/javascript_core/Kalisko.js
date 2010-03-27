function isset(x) {
	return typeof(x) !== "undefined";
}

var kalisko = {
	logError: function(message)
	{
		var ret = xcall.invoke(
			'message = "' + message + '", xcall = { function = logError }');

		var parsedRet = parseStore(ret);

		return isset(parsedRet) && isset(parsedRet.success) && parsedRet.success > 0;
	},

	logWarning: function(message)
	{
		var ret = xcall.invoke(
			'message = "' + message + '", xcall = { function = logWarning }');

		var parsedRet = parseStore(ret);

		return isset(parsedRet) && isset(parsedRet.success) && parsedRet.success > 0;
	},

	logInfo: function(message)
	{
		var ret = xcall.invoke(
			'message = "' + message + '", xcall = { function = logInfo }');

		var parsedRet = parseStore(ret);

		return isset(parsedRet) && isset(parsedRet.success) && parsedRet.success > 0;
	},

	logDebug: function(message)
	{
		var ret = xcall.invoke(
			'message = "' + message + '", xcall = { function = logDebug }');

		var parsedRet = parseStore(ret);

		return isset(parsedRet) && isset(parsedRet.success) && parsedRet.success > 0;
	}
}
