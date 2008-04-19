package emoticons;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

public class EmoFormat
{
	public final List<Emoticon> emoticons;
	public final List<String> protocols;
	
	public EmoFormat(List<Emoticon> emoticons, List<String> protocols)
	{
		this.emoticons = emoticons;
		this.protocols = protocols;
	}
	
	public void load()
	{
		File path = new File("data/");
		for (File file : path.listFiles())
		{
			if (!file.getName().endsWith(".emo"))
				continue;
			
			String protocol = file.getName();
			protocol = protocol.substring(0, protocol.length() - 4);
			protocols.add(protocol);
			try
			{
				BufferedReader in = new BufferedReader(new FileReader(file));
				String line;
				while ((line = in.readLine()) != null)
				{
					line = line.trim();
					if (line.length() <= 0 || line.startsWith("#"))
						continue;
					
					int pos = line.indexOf('=');
					if (pos < 0)
						continue;
					
					String name = removeSeparators(line.substring(0, pos));
					Emoticon emo = getEmoticon(name);
					
					String description = null;
					int endPos = line.indexOf(',', pos + 1);
					if (endPos >= 0)
						description = removeSeparators(line.substring(pos + 1, endPos));
					
					emo.icons.put(protocol, new EmoticonImage(emo, protocol, description));
				}
			}
			catch (IOException e)
			{
				e.printStackTrace();
			}
		}
		
		Collections.sort(emoticons, new Comparator<Emoticon>() {
			public int compare(Emoticon o1, Emoticon o2)
			{
				return o1.name.compareToIgnoreCase(o2.name);
			}
		});
	}
	
	private String removeSeparators(String str)
	{
		str = str.trim();
		if (str.endsWith(","))
		{
			str = str.substring(0, str.length() - 1);
			str = str.trim();
		}
		if (str.charAt(0) == '"' && str.charAt(str.length() - 1) == '"')
			str = str.substring(1, str.length() - 1);
		return str;
	}
	
	private Emoticon getEmoticon(String name)
	{
		for (Emoticon emoticon : emoticons)
		{
			if (emoticon.name.equals(name))
				return emoticon;
		}
		Emoticon emo = new Emoticon();
		emo.name = name;
		emo.icons.put(null, new EmoticonImage(emo, null));
		emoticons.add(emo);
		return emo;
	}
	
}
