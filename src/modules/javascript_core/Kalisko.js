var kalisko = {
	logError: function(text)
	{
		var ret = xcall.invoke(
			'text = "' + text + '", xcall = { function = logError }');

		return ret.success > 0;
	},

	logWarning: function(text)
	{
		var ret = xcall.invoke(
			'text = "' + text + '", xcall = { function = logWarning }');

		return ret.success > 0;
	},

	logInfo: function(text)
	{
		var ret = xcall.invoke(
			'text = "' + text + '", xcall = { function = logInfo }');

		return ret.success > 0;
	},

	logDebug: function(text)
	{
		var ret = xcall.invoke(
			'text = "' + text + '", xcall = { function = logDebug }');

		return ret.success > 0;
	}
}
