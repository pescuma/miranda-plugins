package org.pescuma.miricogen;

import static org.pescuma.miricogen.ImageUtils.*;

import java.awt.image.BufferedImage;

import org.pescuma.miricogen.ImageUtils.RGBA;

public class BrightnessTransform implements ImageTransform
{
	private final int brightness;
	
	public BrightnessTransform(int brightness)
	{
		this.brightness = brightness;
	}
	
	public void transform(BufferedImage img)
	{
		for (int x = 0; x < img.getWidth(); x++)
		{
			for (int y = 0; y < img.getHeight(); y++)
			{
				RGBA c = getRGBA(img, x, y);
				
				c.r += brightness;
				c.g += brightness;
				c.b += brightness;
				
				setRGBA(img, x, y, c);
			}
		}
	}
}
