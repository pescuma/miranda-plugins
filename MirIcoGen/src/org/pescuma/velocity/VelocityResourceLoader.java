package org.pescuma.velocity;


import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;

import org.apache.commons.collections.ExtendedProperties;
import org.apache.velocity.exception.ResourceNotFoundException;
import org.apache.velocity.io.UnicodeInputStream;
import org.apache.velocity.runtime.resource.Resource;
import org.apache.velocity.runtime.resource.loader.ResourceLoader;

public class VelocityResourceLoader extends ResourceLoader
{
	/** Shall we inspect unicode files to see what encoding they contain?. */
	private boolean unicode = false;
	
	@Override
	public void init(ExtendedProperties configuration)
	{
		// unicode files may have a BOM marker at the start, but Java
		// has problems recognizing the UTF-8 bom. Enabling unicode will
		// recognize all unicode boms.
		unicode = configuration.getBoolean("unicode", false);
	}
	
	@Override
	public InputStream getResourceStream(String templateName) throws ResourceNotFoundException
	{
		if (templateName == null)
			throw new ResourceNotFoundException("Need to specify a file name or file path!");
		
		templateName = templateName.trim();
		if ("".equals(templateName))
			throw new ResourceNotFoundException("Need to specify a file name or file path!");
		
		File file = new File(templateName);
		if (file.canRead())
			return getInputStream(file);
		
		InputStream is = VelocityResourceLoader.class.getClassLoader().getResourceAsStream(templateName);
		if (is != null)
			return is;
		
		throw new ResourceNotFoundException("Cannot find " + templateName);
	}
	
	private InputStream getInputStream(File file) throws ResourceNotFoundException
	{
		FileInputStream fis = null;
		UnicodeInputStream uis = null;
		try
		{
			fis = new FileInputStream(file.getAbsolutePath());
			
			if (unicode)
			{
				uis = new UnicodeInputStream(fis, true);
				return new BufferedInputStream(uis);
			}
			else
			{
				return new BufferedInputStream(fis);
			}
		}
		catch (IOException e)
		{
			closeQuiet(uis);
			closeQuiet(fis);
			throw new ResourceNotFoundException(e);
		}
	}
	
	private void closeQuiet(final InputStream is)
	{
		if (is != null)
		{
			try
			{
				is.close();
			}
			catch (IOException ioe)
			{
				// Ignore
			}
		}
	}
	
	@Override
	public boolean isSourceModified(Resource resource)
	{
		return false;
	}
	
	@Override
	public long getLastModified(Resource resource)
	{
		return 0;
	}
	
}
