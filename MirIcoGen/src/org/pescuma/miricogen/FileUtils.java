package org.pescuma.miricogen;

import java.io.File;
import java.io.IOException;

public class FileUtils
{
	public static String getFullPath(File overlayIco)
	{
		try
		{
			return overlayIco.getCanonicalPath();
		}
		catch (IOException e)
		{
			return overlayIco.getAbsolutePath();
		}
	}
	
}
