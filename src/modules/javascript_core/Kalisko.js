var kalisko = {
	logError: function(message)
	{
		var ret = xcall.invoke(
			'message = "' + message + '", xcall = { function = logError }');

		return ret.success > 0;
	},

	logWarning: function(message)
	{
		var ret = xcall.invoke(
			'message = "' + message + '", xcall = { function = logWarning }');

		return ret.success > 0;
	},

	logInfo: function(message)
	{
		var ret = xcall.invoke(
			'message = "' + message + '", xcall = { function = logInfo }');

		return ret.success > 0;
	},

	logDebug: function(message)
	{
		var ret = xcall.invoke(
			'message = "' + message + '", xcall = { function = logDebug }');

		return ret.success > 0;
	}
}
