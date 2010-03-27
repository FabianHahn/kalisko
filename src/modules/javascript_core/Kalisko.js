function isset(x) {
	return typeof(x) !== "undefined";
}

var kalisko = {
	logError: function(text)
	{
		var ret = xcall.invoke(
			'error = "' + text + '", xcall = { function = logError }');

		var parsedRet = parseStore(ret);

		return isset(parsedRet) && isset(parsedRet.success) && parsedRet.success > 0;
	},

	logWarning: function(text)
	{
		var ret = xcall.invoke(
			'warning = "' + text + '", xcall = { function = logWarning }');

		var parsedRet = parseStore(ret);

		return true;
	},

	logInfo: function(text)
	{
		var ret = xcall.invoke(
			'info = "' + text + '", xcall = { function = logInfo }');

		var parsedRet = parseStore(ret);

		return isset(parsedRet) && isset(parsedRet.success) && parsedRet.success > 0;
	},

	logDebug: function(text)
	{
		var ret = xcall.invoke(
			'debug = "' + text + '", xcall = { function = logDebug }');

		var parsedRet = parseStore(ret);

		return isset(parsedRet) && isset(parsedRet.success) && parsedRet.success > 0;
	}
}
