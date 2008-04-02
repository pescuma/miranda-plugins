package emoticons;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

public class Mep
{
	public File path;
	public String name = "";
	public String creator = "";
	public String udaterURL = "";
	public List<Emoticon> emoticons;
	public final List<ImageFile> images = new ArrayList<ImageFile>();
	
	public Emoticon getEmoticon(String emoName, boolean create)
	{
		for (Emoticon emo : emoticons)
			if (emo.name.equalsIgnoreCase(emoName))
				return emo;
		
		if (create)
		{
			Emoticon emo = new Emoticon();
			emo.name = emoName;
			emo.icons.put(null, new EmoticonImage());
			emoticons.add(emo);
			return emo;
		}
		
		return null;
	}
	
	public ImageFile getImage(String imgPath)
	{
		for (ImageFile img : images)
			if (img.path.equalsIgnoreCase(imgPath))
				return img;
		return null;
	}
	
	public ImageFile createImage(String imgPath, String protocol)
	{
		return newImage(imgPath, protocol, true);
	}
	
	public ImageFile createTemporaryImage(String imgPath, String protocol)
	{
		return newImage(imgPath, protocol, false);
	}
	
	private ImageFile newImage(String imgPath, String protocol, boolean addToList)
	{
		String realPath = imgPath;
		if (realPath.startsWith("http://"))
		{
			String p = realPath.substring(realPath.lastIndexOf('/') + 1);
			if (protocol == null)
				realPath = "cache\\" + p;
			else
				realPath = "cache\\" + protocol + "\\" + p;
		}
		File imgRealPath = new File(path, realPath);
		
		ImageFile image = null;
		for (ImageFile img : images)
		{
			if (img.path.equalsIgnoreCase(imgPath))
			{
				image = img;
				break;
			}
		}
		
		if (image == null)
		{
			image = new ImageFile(imgPath, imgRealPath);
			if (addToList)
				images.add(image);
		}
		else
		{
			image.realPath = imgRealPath;
		}
		
		return image;
	}
	
	public void setEmoticonImage(String protocol, String name, ImageFile image)
	{
		Emoticon emo = getEmoticon(name, false);
		if (emo == null)
			return;
		
		if (protocol == null)
		{
			for (EmoticonImage icon : emo.icons.values())
			{
				icon.image = image;
			}
		}
		else
		{
			EmoticonImage icon = emo.icons.get(protocol);
			if (icon == null)
				return;
			icon.image = image;
		}
	}
}
