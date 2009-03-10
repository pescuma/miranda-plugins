package org.pescuma.miricogen;

import java.io.File;

public class PluginIcon
{
	private final String name;
	private final int id;
	private final File path;
	
	public PluginIcon(File path, String name, int id)
	{
		this.path = path;
		this.name = name;
		this.id = id;
	}
	
	public String getName()
	{
		return name;
	}
	
	public int getId()
	{
		return id;
	}
	
	public File getFile()
	{
		return ImageUtils.findIcon(new File(path, name));
	}
	
	public boolean exists()
	{
		File file = getFile();
		return file.exists() && file.isFile();
	}
}
