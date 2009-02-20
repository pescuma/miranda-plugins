package org.pescuma.miricogen;

import static java.lang.Math.*;
import static org.pescuma.miricogen.ImageUtils.*;

import java.awt.image.BufferedImage;

import org.pescuma.miricogen.ImageUtils.RGBA;

public class GrayscaleTransform implements ImageTransform
{
	public void transform(BufferedImage img)
	{
		for (int x = 0; x < img.getWidth(); x++)
		{
			for (int y = 0; y < img.getHeight(); y++)
			{
				RGBA c = getRGBA(img, x, y);
				
				int gray = round(computeLuminescense(c));
//				int gray = round((c.r + c.g + c.b) / 3f);
				c.r = c.g = c.b = gray;
				
				setRGBA(img, x, y, c);
			}
		}
	}
}
