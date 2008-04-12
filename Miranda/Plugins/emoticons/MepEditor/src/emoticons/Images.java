package emoticons;

import java.io.File;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

import org.eclipse.swt.SWTException;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;

public class Images
{
	private static Map<String, Image> images = new HashMap<String, Image>();
	
	public static Image get(String filename)
	{
		if (images.containsKey(filename))
			return images.get(filename);
		
		Image ret;
		try
		{
			ret = new Image(Display.getCurrent(), filename);
		}
		catch (SWTException e)
		{
			ret = null;
		}
		images.put(filename, ret);
		return ret;
	}
	
	public static Image get(File file)
	{
		try
		{
			return get(file.getCanonicalPath());
		}
		catch (IOException e)
		{
			e.printStackTrace();
			return null;
		}
	}
}
