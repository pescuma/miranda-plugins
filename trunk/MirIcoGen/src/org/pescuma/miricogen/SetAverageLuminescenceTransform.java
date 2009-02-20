package org.pescuma.miricogen;

import static java.lang.Math.*;
import static org.pescuma.miricogen.ImageUtils.*;

import java.awt.image.BufferedImage;

import org.pescuma.miricogen.ImageUtils.RGBA;

public class SetAverageLuminescenceTransform implements ImageTransform
{
	private final int desired;
	
	public SetAverageLuminescenceTransform(int desired)
	{
		this.desired = desired;
	}
	
	public void transform(BufferedImage img)
	{
		LumiData lumi = computeLumiData(img);
		
		if (lumi.pixels < 0.001f)
			return; // All transparent
			
		float delta = desired - lumi.avgLumi;
		
		for (int iter = 0; iter < 5 && (delta <= -1f || delta >= 1f); iter++)
		{
			if (delta > 0)
				delta = (delta * lumi.pixels) / lumi.nonWhitePixels;
			else
				delta = (delta * lumi.pixels) / lumi.nonBlackPixels;
			
			applyDiffToColors(img, round(delta));
			
			lumi = computeLumiData(img);
			delta = desired - lumi.avgLumi;
		}
	}
	
	private void applyDiffToColors(BufferedImage img, int d)
	{
		for (int x = 0; x < img.getWidth(); x++)
		{
			for (int y = 0; y < img.getHeight(); y++)
			{
				RGBA c = getRGBA(img, x, y);
				
				c.r += d;
				c.g += d;
				c.b += d;
				
				setRGBA(img, x, y, c);
			}
		}
	}
	
	private LumiData computeLumiData(BufferedImage img)
	{
		LumiData lumi = new LumiData();
		for (int x = 0; x < img.getWidth(); x++)
		{
			for (int y = 0; y < img.getHeight(); y++)
			{
				RGBA c = getRGBA(img, x, y);
				
				float a = c.a / 255f;
				lumi.lumi += round(computeLuminescense(c)) * a;
				lumi.pixels += a;
				
				if (c.r != 0 || c.g != 0 || c.b != 0)
					lumi.nonBlackPixels += a;
				
				if (c.r != 255 || c.g != 255 || c.b != 255)
					lumi.nonWhitePixels += a;
				
				setRGBA(img, x, y, c);
			}
		}
		
		lumi.avgLumi = lumi.lumi / lumi.pixels;
		
		return lumi;
	}
	
	private static class LumiData
	{
		float avgLumi;
		float lumi;
		float pixels;
		float nonWhitePixels;
		float nonBlackPixels;
	}
}
