package org.pescuma.miricogen;

import static java.lang.Math.*;

import java.awt.image.BufferedImage;
import java.awt.image.ColorModel;
import java.awt.image.DirectColorModel;
import java.awt.image.IndexColorModel;
import java.awt.image.WritableRaster;
import java.io.File;
import java.io.FilenameFilter;
import java.io.IOException;
import java.util.List;

import net.sf.image4j.codec.ico.ICODecoder;

import org.eclipse.swt.SWTException;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.PaletteData;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.widgets.Display;

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
		
		return ret;
	}
	
	public static float computeLuminescense(RGBA c)
	{
		return c.r * 0.299f + c.g * 0.587f + c.b * 0.114f;
	}
	
	public static Image loadImage(File file)
	{
		return loadImage(file, -1, -1, true);
	}
	
	public static Image loadImage(File file, boolean exceptionOnError)
	{
		return loadImage(file, -1, -1, exceptionOnError);
	}
	
	public static Image loadImage(File file, int width, int height)
	{
		return loadImage(file, width, height, true);
	}
	
	public static Image loadImage(File file, int width, int height, boolean exceptionOnError)
	{
		Image ret = null;
		
		if (file.getName().substring(file.getName().length() - 4).equals(".ico"))
		{
			try
			{
				BufferedImage icoImg = null;
				List<BufferedImage> protoImgs = ICODecoder.read(file);
				if (width > 0 && height > 0)
					icoImg = findBestImage(protoImgs, width, height);
				else
					icoImg = protoImgs.get(0);
				
				if (exceptionOnError && icoImg == null)
				{
					if (width > 0 && height > 0)
						throw new IllegalStateException("No icon with " + width + "x" + height);
					else
						throw new IllegalArgumentException("Could not find ico inside .ico");
				}
				
				if (icoImg != null)
					ret = new Image(Display.getDefault(), convertToSWT(icoImg));
			}
			catch (IOException e)
			{
				e.printStackTrace();
			}
		}
		
		if (ret == null)
		{
			try
			{
				ret = new Image(Display.getDefault(), file.getAbsolutePath());
			}
			catch (SWTException e)
			{
				e.printStackTrace();
				
				if (exceptionOnError)
					throw new IllegalStateException("Could not load image: " + file.getAbsolutePath(), e);
			}
		}
		
		return ret;
	}
	
	public static ImageData convertToSWT(BufferedImage bufferedImage)
	{
		if (bufferedImage.getColorModel() instanceof DirectColorModel)
		{
			DirectColorModel colorModel = (DirectColorModel) bufferedImage.getColorModel();
			PaletteData palette = new PaletteData(colorModel.getRedMask(), colorModel.getGreenMask(), colorModel.getBlueMask());
			ImageData data = new ImageData(bufferedImage.getWidth(), bufferedImage.getHeight(), colorModel.getPixelSize(), palette);
			WritableRaster raster = bufferedImage.getRaster();
			int[] pixelArray = new int[4];
			for (int y = 0; y < data.height; y++)
			{
				for (int x = 0; x < data.width; x++)
				{
					raster.getPixel(x, y, pixelArray);
					int pixel = palette.getPixel(new RGB(pixelArray[0], pixelArray[1], pixelArray[2]));
					data.setPixel(x, y, pixel);
					data.setAlpha(x, y, pixelArray[3]);
				}
			}
			return data;
		}
		else if (bufferedImage.getColorModel() instanceof IndexColorModel)
		{
			IndexColorModel colorModel = (IndexColorModel) bufferedImage.getColorModel();
			int size = colorModel.getMapSize();
			byte[] reds = new byte[size];
			byte[] greens = new byte[size];
			byte[] blues = new byte[size];
			colorModel.getReds(reds);
			colorModel.getGreens(greens);
			colorModel.getBlues(blues);
			RGB[] rgbs = new RGB[size];
			for (int i = 0; i < rgbs.length; i++)
			{
				rgbs[i] = new RGB(reds[i] & 0xFF, greens[i] & 0xFF, blues[i] & 0xFF);
			}
			PaletteData palette = new PaletteData(rgbs);
			ImageData data = new ImageData(bufferedImage.getWidth(), bufferedImage.getHeight(), colorModel.getPixelSize(), palette);
			data.transparentPixel = colorModel.getTransparentPixel();
			WritableRaster raster = bufferedImage.getRaster();
			int[] pixelArray = new int[1];
			for (int y = 0; y < data.height; y++)
			{
				for (int x = 0; x < data.width; x++)
				{
					raster.getPixel(x, y, pixelArray);
					data.setPixel(x, y, pixelArray[0]);
				}
			}
			return data;
		}
		return null;
	}
	
	public static BufferedImage convertToAWT(ImageData data)
	{
		ColorModel colorModel = null;
		PaletteData palette = data.palette;
		if (palette.isDirect)
		{
			colorModel = new DirectColorModel(32, 0xFF << 16, 0xFF << 8, 0xFF, 0xFF << 24);
			BufferedImage bufferedImage = new BufferedImage(colorModel, colorModel.createCompatibleWritableRaster(data.width, data.height),
					false, null);
			WritableRaster raster = bufferedImage.getRaster();
			int[] pixelArray = new int[4];
			for (int y = 0; y < data.height; y++)
			{
				for (int x = 0; x < data.width; x++)
				{
					int pixel = data.getPixel(x, y);
					RGB rgb = palette.getRGB(pixel);
					pixelArray[0] = rgb.red;
					pixelArray[1] = rgb.green;
					pixelArray[2] = rgb.blue;
					pixelArray[3] = data.getAlpha(x, y);
					raster.setPixels(x, y, 1, 1, pixelArray);
				}
			}
			return bufferedImage;
		}
		else
		{
			RGB[] rgbs = palette.getRGBs();
			byte[] red = new byte[rgbs.length];
			byte[] green = new byte[rgbs.length];
			byte[] blue = new byte[rgbs.length];
			for (int i = 0; i < rgbs.length; i++)
			{
				RGB rgb = rgbs[i];
				red[i] = (byte) rgb.red;
				green[i] = (byte) rgb.green;
				blue[i] = (byte) rgb.blue;
			}
			if (data.transparentPixel != -1)
			{
				colorModel = new IndexColorModel(data.depth, rgbs.length, red, green, blue, data.transparentPixel);
			}
			else
			{
				colorModel = new IndexColorModel(data.depth, rgbs.length, red, green, blue);
			}
			BufferedImage bufferedImage = new BufferedImage(colorModel, colorModel.createCompatibleWritableRaster(data.width, data.height),
					false, null);
			WritableRaster raster = bufferedImage.getRaster();
			int[] pixelArray = new int[1];
			for (int y = 0; y < data.height; y++)
			{
				for (int x = 0; x < data.width; x++)
				{
					int pixel = data.getPixel(x, y);
					pixelArray[0] = pixel;
					raster.setPixel(x, y, pixelArray);
				}
			}
			return bufferedImage;
		}
	}
	
	public static final String[] imageExts = new String[] { ".ico", ".png", ".gif", ".jpg", ".bmp" };
	
	public static File findIcon(File base)
	{
		final String baseName = base.getName();
		final int baseLen = baseName.length();
		
		File path = base.getParentFile();
		File[] files = path.listFiles(new FilenameFilter() {
			public boolean accept(File dir, String name)
			{
				int length = name.length();
				if (length != baseLen + 4)
					return false;
				if (!baseName.equals(name.substring(0, baseLen)))
					return false;
				
				String ext = name.substring(length - 4);
				return indexOf(imageExts, ext) >= 0;
			}
		});
		File ret = null;
		int min = imageExts.length;
		for (File file : files)
		{
			String name = file.getName();
			String ext = name.substring(name.length() - 4);
			int i = indexOf(imageExts, ext);
			
			if (i < min)
			{
				min = i;
				ret = file;
			}
		}
		
		if (ret == null)
			ret = new File(path, base.getName() + ".ico");
		
		return ret;
	}
	
	public static int indexOf(String[] arr, String val)
	{
		for (int i = 0; i < arr.length; i++)
		{
			if (arr[i].equals(val))
				return i;
		}
		return -1;
	}
}
