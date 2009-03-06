package org.pescuma.miricogen;

import java.io.File;

public class PluginIcon
{
	private final String name;
	private final int id;
	private final File file;
	
	public PluginIcon(File path, String name, int id)
	{
		this.name = name;
		this.id = id;
		file = new File(path, name + ".ico");
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
		return file;
	}
	
	public boolean exists()
	{
		return file.exists() && file.isFile();
	}
}
