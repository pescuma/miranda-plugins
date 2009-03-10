package org.pescuma.miricogen;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class Plugin
{
	private final String name;
	private final File path;
	private final List<PluginIcon> icons = new ArrayList<PluginIcon>();
	private final int existingIcons;
	
	public Plugin(String name, File path)
	{
		this.name = name;
		this.path = path;
		
		existingIcons = loadIcons();
	}
	
	public String getName()
	{
		return name;
	}
	
	public File getPath()
	{
		return path;
	}
	
	public List<PluginIcon> getIcons()
	{
		return icons;
	}
	
	public int getExistingIcons()
	{
		return existingIcons;
	}
	
	private int loadIcons()
	{
		File iconsFile = new File(path, name + ".icons");
		if (!iconsFile.exists() || !iconsFile.isFile())
			return 0;
		
		try
		{
			int ret = 0;
			
			BufferedReader in = new BufferedReader(new FileReader(iconsFile));
			String line;
			while ((line = in.readLine()) != null)
			{
				int pos = line.indexOf('#');
				if (pos >= 0)
					line = line.substring(0, pos);
				
				line = line.replaceAll("\\s+", " ").trim().toLowerCase();
				if (line.isEmpty())
					continue;
				
				String[] tokens = line.split("=");
				if (tokens.length != 2)
					continue;
				
				String iconName = tokens[0].trim();
				if (iconName.isEmpty())
					continue;
				
				String iconID = tokens[1];
				if (!isInteger(iconID))
					continue;
				
				PluginIcon icon = new PluginIcon(path, iconName, Integer.parseInt(iconID));
				if (icon.exists())
					ret++;
				
				icons.add(icon);
			}
			
			return ret;
		}
		catch (IOException e)
		{
			e.printStackTrace();
			
			return 0;
		}
	}
	
	private boolean isInteger(String string)
	{
		try
		{
			Integer.parseInt(string);
			return true;
		}
		catch (NumberFormatException e)
		{
			return false;
		}
	}
	
	public int countExistingIcons()
	{
		int ret = 0;
		for (PluginIcon ico : icons)
		{
			if (ico.exists())
				ret++;
		}
		return ret;
	}
	
}
