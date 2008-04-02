package emoticons;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import org.eclipse.swt.SWT;
import org.eclipse.swt.SWTException;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.ImageLoader;
import org.eclipse.swt.widgets.Display;

public class ImageUtils
{
	
	public static Image[] getFrames(File file)
	{
		List<Image> frames = new ArrayList<Image>();
		
		try
		{
			Color shellBackground = MepEditor.shell.getBackground();
			
			ImageLoader loader = new ImageLoader();
			ImageData[] imageDataArray = loader.load(file.getCanonicalPath());
			if (imageDataArray.length == 1)
				return new Image[] { new Image(Display.getCurrent(), imageDataArray[0]) };
			
			Image fullGif = new Image(Display.getCurrent(), loader.logicalScreenWidth, loader.logicalScreenHeight);
			GC fullGifGC = new GC(fullGif);
			fullGifGC.setBackground(shellBackground);
			fullGifGC.fillRectangle(0, 0, loader.logicalScreenWidth, loader.logicalScreenHeight);
			
			for (ImageData imageData : imageDataArray)
			{
				Image image = new Image(Display.getCurrent(), imageData);
				fullGifGC.drawImage(image, 0, 0, imageData.width, imageData.height, imageData.x, imageData.y, imageData.width,
						imageData.height);
				
				Image frame = new Image(Display.getCurrent(), loader.logicalScreenWidth, loader.logicalScreenHeight);
				GC frameGC = new GC(frame);
				frameGC.drawImage(fullGif, 0, 0);
				frameGC.dispose();
				frames.add(frame);
				
				switch (imageData.disposalMethod)
				{
					case SWT.DM_FILL_BACKGROUND:
						Color bgColor = null;
						if (loader.backgroundPixel != -1)
							bgColor = new Color(Display.getCurrent(), imageData.palette.getRGB(loader.backgroundPixel));
						fullGifGC.setBackground(bgColor != null ? bgColor : shellBackground);
						fullGifGC.fillRectangle(imageData.x, imageData.y, imageData.width, imageData.height);
						if (bgColor != null)
							bgColor.dispose();
						break;
				}
			}
			
			fullGifGC.dispose();
			fullGif.dispose();
		}
		catch (SWTException e)
		{
			e.printStackTrace();
		}
		catch (IOException e)
		{
			e.printStackTrace();
		}
		
		return frames.toArray(new Image[0]);
	}
	
}
