package org.pescuma.velocity;

import org.apache.velocity.runtime.RuntimeServices;
import org.apache.velocity.runtime.log.LogChute;

public class VelocityLogger implements LogChute
{
	public void init(RuntimeServices rsvc) throws Exception
	{
	}
	
	public boolean isLevelEnabled(int level)
	{
		return level == WARN_ID || level == ERROR_ID;
	}
	
	public void log(int level, String message)
	{
		if (!isLevelEnabled(level))
			return;
		System.err.println("[Velocity] [" + toString(level) + "] " + message);
	}
	
	public void log(int level, String message, Throwable t)
	{
		if (!isLevelEnabled(level))
			return;
		System.err.println("[Velocity] [" + toString(level) + "] " + message);
		t.printStackTrace();
	}
	
	private String toString(int level)
	{
		switch (level)
		{
			case TRACE_ID:
				return "TRACE";
			case DEBUG_ID:
				return "DEBUG";
			case INFO_ID:
				return "INFO";
			case WARN_ID:
				return "WARN";
			case ERROR_ID:
				return "ERROR";
		}
		return "-";
	}
	
}
