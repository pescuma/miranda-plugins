package org.pescuma.miricogen;

import static java.lang.Math.*;

import java.awt.image.BufferedImage;
import java.util.List;

public class ImageUtils
{
	public static class RGBA
	{
		public int r;
		public int g;
		public int b;
		public int a;
		
		public RGBA(int r, int g, int b, int a)
		{
			this.r = r;
			this.g = g;
			this.b = b;
			this.a = a;
		}
	}
	
	public static RGBA getRGBA(BufferedImage data, int x, int y)
	{
		int pixel = data.getRGB(x, y);
//		System.out.println(String.format("g %x", pixel));
		return new RGBA((pixel >> 16) & 0xff, (pixel >> 8) & 0xff, pixel & 0xff, (pixel >> 24) & 0xff);
	}
	
	public static void setRGBA(BufferedImage data, int x, int y, RGBA rgba)
	{
		if (x < 0 || x >= data.getWidth() || y < 0 || y >= data.getHeight())
			return;
		
		int r = max(0, min(255, rgba.r));
		int g = max(0, min(255, rgba.g));
		int b = max(0, min(255, rgba.b));
		int a = max(0, min(255, rgba.a));
		
		int pixel = (r << 16) | (g << 8) | b | (a << 24);
//		System.out.println(String.format("s %x", pixel));
		data.setRGB(x, y, pixel);
	}
	
	public static void drawOver(BufferedImage proto, BufferedImage overlay)
	{
		// http://en.wikipedia.org/wiki/Alpha_compositing
		
		for (int x = 0; x < proto.getWidth(); x++)
		{
			for (int y = 0; y < proto.getHeight(); y++)
			{
				RGBA b = getRGBA(proto, x, y);
				float ba = b.a / 255f;
				
				RGBA o = getRGBA(overlay, x, y);
				float oa = o.a / 255f;
				
				float aa = drawOverAlpha(ba, oa);
				b.r = drawOver(b.r, ba, o.r, oa, aa);
				b.g = drawOver(b.g, ba, o.g, oa, aa);
				b.b = drawOver(b.b, ba, o.b, oa, aa);
				b.a = round(aa * 255);
				
				setRGBA(proto, x, y, b);
			}
		}
	}
	
	private static int drawOver(int b, float ba, int o, float oa, float aa)
	{
		return round((o * oa + b * ba * (1 - oa)) / aa);
	}
	
	private static float drawOverAlpha(float ba, float oa)
	{
		return oa + ba * (1 - oa);
	}
	
	public static BufferedImage findBestImage(List<BufferedImage> images, int width, int height)
	{
		if (images == null || images.size() < 0)
			throw new IllegalStateException("Could not read protocol icon");
		
		int max = 0;
		BufferedImage ret = null;
		
		for (BufferedImage img : images)
		{
			if (img.getWidth() != width || img.getHeight() != height)
				continue;
			
			int bpp = img.getColorModel().getPixelSize();
			if (bpp > max)
			{
				bpp = max;
				ret = img;
			}
		}
		
		if (ret == null)
			throw new IllegalStateException("No icon with " + width + "x" + height);
		
		return ret;
	}
	
	public static float computeLuminescense(RGBA c)
	{
		return c.r * 0.299f + c.g * 0.587f + c.b * 0.114f;
	}
}
