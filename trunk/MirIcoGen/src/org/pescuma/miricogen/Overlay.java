package org.pescuma.miricogen;

import static org.pescuma.miricogen.ImageUtils.*;

import java.awt.image.BufferedImage;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import net.sf.image4j.codec.ico.ICODecoder;

import org.pescuma.miricogen.Main.Config;

public class Overlay
{
	public final List<ImageTransform> protoTransforms = new ArrayList<ImageTransform>();
	public final List<ImageTransform> overlayTransforms = new ArrayList<ImageTransform>();
	public final List<ImageTransform> outTransforms = new ArrayList<ImageTransform>();
	
	private final Config cfg;
	private final String name;
	private BufferedImage overlay;
	private boolean loadedOverlay;
	private boolean loadedTransforms;
	
	public Overlay(Config cfg, String name)
	{
		this.cfg = cfg;
		this.name = name;
	}
	
	public String getName()
	{
		return name;
	}
	
	public void blend(BufferedImage proto)
	{
		loadTransforms();
		loadOverlay();
		
		for (ImageTransform transf : protoTransforms)
			transf.transform(proto);
		
		if (overlay != null)
			ImageUtils.drawOver(proto, overlay);
		
		for (ImageTransform transf : outTransforms)
			transf.transform(proto);
	}
	
	public boolean isEmpty()
	{
		loadTransforms();
		loadOverlay();
		return overlay == null && protoTransforms.size() <= 0 && outTransforms.size() <= 0;
	}
	
	private void loadTransforms()
	{
		if (loadedTransforms)
			return;
		loadedTransforms = true;
		
		File file = new File(cfg.paths.overlaysPath, name + ".transforms");
		if (!file.exists())
			return;
		
		try
		{
			BufferedReader in = new BufferedReader(new FileReader(file));
			String line;
			while ((line = in.readLine()) != null)
			{
				int pos = line.indexOf('#');
				if (pos >= 0)
					line = line.substring(0, pos);
				
				line = line.replaceAll("\\s+", " ").trim().toLowerCase();
				if (line.isEmpty())
					continue;
				
				String[] tokens = line.split(" ");
				
				ImageTransform transf = loadTransform(tokens, 1);
				if (transf == null)
					continue;
				
				if ("proto".equals(tokens[0]))
					protoTransforms.add(transf);
				else if ("overlay".equals(tokens[0]))
					overlayTransforms.add(transf);
				else if ("out".equals(tokens[0]))
					outTransforms.add(transf);
			}
		}
		catch (IOException e)
		{
			e.printStackTrace();
		}
	}
	
	private ImageTransform loadTransform(String[] tokens, int i)
	{
		if ("grayscale".equals(tokens[i]))
			return new GrayscaleTransform();
		
		else if ("set_avg_lumi".equals(tokens[i]))
			return new SetAverageLuminescenceTransform(toInt(tokens[i + 1]));
		
		else if ("bright".equals(tokens[i]))
			return new BrightnessTransform(toInt(tokens[i + 1]));
		
		else if ("transp".equals(tokens[i]))
			return new TransparencyTransform(toInt(tokens[i + 1]));
		
		else
			return null;
	}
	
	private int toInt(String token)
	{
		token = token.trim();
		if (token.charAt(0) == '+')
			token = token.substring(1);
		return Integer.parseInt(token);
	}
	
	private void loadOverlay()
	{
		if (loadedOverlay)
			return;
		loadedOverlay = true;
		
		loadTransforms();
		
		File file = new File(cfg.paths.overlaysPath, name + ".ico");
		if (!file.exists())
			return;
		
		try
		{
			overlay = findBestImage(ICODecoder.read(file), 16, 16);
		}
		catch (IOException e)
		{
			throw new RuntimeException(e);
		}
		
		for (ImageTransform transf : overlayTransforms)
			transf.transform(overlay);
	}
	
}
