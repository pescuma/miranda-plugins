package org.pescuma.miricogen;

import static org.pescuma.miricogen.ImageUtils.*;

import java.awt.image.BufferedImage;

import org.pescuma.miricogen.ImageUtils.RGBA;

public class TransparencyTransform implements ImageTransform
{
	private final int transparency;
	
	public TransparencyTransform(int transparency)
	{
		this.transparency = transparency;
	}
	
	public void transform(BufferedImage img)
	{
		for (int x = 0; x < img.getWidth(); x++)
		{
			for (int y = 0; y < img.getHeight(); y++)
			{
				RGBA c = getRGBA(img, x, y);
				
				c.a -= transparency;
				
				setRGBA(img, x, y, c);
			}
		}
	}
}
