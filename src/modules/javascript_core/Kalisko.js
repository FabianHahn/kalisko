var kalisko = {
	logError: function(message)
	{
		var call = {
			message: message,
			xcall: {
				function: "logError"
			}
		};
		
		var ret = xcall.invoke(call);

		return ret.success > 0;
	},

	logWarning: function(message)
	{
		var call = {
			message: message,
			xcall: {
				function: "logWarning"
			}
		};
		
		var ret = xcall.invoke(call);

		return ret.success > 0;
	},

	logInfo: function(message)
	{
		var call = {
			message: message,
			xcall: {
				function: "logInfo"
			}
		};
		
		var ret = xcall.invoke(call);

		return ret.success > 0;
	},

	logDebug: function(message)
	{
		var call = {
			message: message,
			xcall: {
				function: "logDebug"
			}
		};
		
		var ret = xcall.invoke(call);

		return ret.success > 0;
	}
}
