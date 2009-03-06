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
	
	public Plugin(String name, File path)
	{
		this.name = name;
		this.path = path;
		
		loadIcons();
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
	
	private void loadIcons()
	{
		File iconsFile = new File(path, name + ".icons");
		if (!iconsFile.exists() || !iconsFile.isFile())
			return;
		
		try
		{
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
				if (!icon.exists())
					continue;
				
				icons.add(icon);
			}
		}
		catch (IOException e)
		{
			e.printStackTrace();
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
	
}
