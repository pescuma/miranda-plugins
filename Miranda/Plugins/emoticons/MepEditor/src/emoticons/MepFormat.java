package emoticons;

import static java.lang.Math.max;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.List;

public class MepFormat
{
	private Mep mep;
	private final List<String> protocols;
	
	public MepFormat(File path, List<Emoticon> emoticons, List<String> protocols)
	{
		mep = new Mep();
		mep.path = path;
		mep.name = path.getName();
		mep.emoticons = emoticons;
		this.protocols = protocols;
	}
	
	public Mep load() throws IOException
	{
		addImages(mep.path, null);
		
		for (String protocol : protocols)
		{
			File path = new File(mep.path, protocol);
			if (!path.exists())
				continue;
			addImages(path, protocol);
		}
		
		File mepFile = new File(mep.path, mep.path.getName() + ".mep");
		if (mepFile.exists())
		{
			BufferedReader in = new BufferedReader(new FileReader(mepFile));
			String line;
			while ((line = in.readLine()) != null)
			{
				line = line.trim();
				if (line.startsWith("#"))
				{
					continue;
				}
				else if (line.length() < 2)
				{
					continue;
				}
				else if (startsWithIgnoreCase(line, "Name:"))
				{
					mep.name = line.substring(5).trim();
				}
				else if (startsWithIgnoreCase(line, "Creator:"))
				{
					mep.creator = line.substring(8).trim();
				}
				else if (startsWithIgnoreCase(line, "Updater URL:"))
				{
					mep.udaterURL = line.substring(12).trim();
				}
				else if (line.startsWith("\""))
				{
					handleMepLine(line);
				}
			}
		}
		
		return mep;
	}
	
	private boolean startsWithIgnoreCase(String line, String txt)
	{
		return line.length() > txt.length() && line.substring(0, txt.length()).compareToIgnoreCase(txt) == 0;
	}
	
	private void handleMepLine(String line)
	{
		int len = line.length();
		int state = 0;
		int pos = 0;
		int module_pos = -1;
		boolean noDelimiter = false;
		
		ImageFile img = null;
		String name = null;
		String protocol = null;
		
		for (int i = 0; i <= len && state < 6; i++)
		{
			char c = (i == len ? '\0' : line.charAt(i));
			if (c == ' ')
				continue;
			
			if ((state % 2) == 0)
			{
				if (c == '"')
				{
					state++;
					pos = i + 1;
					noDelimiter = false;
				}
				else if (c != '=' && c != ',' && c != ' ' && c != '\t' && c != '\r' && c != '\n')
				{
					state++;
					pos = i;
					noDelimiter = true;
				}
				else
				{
					continue;
				}
			}
			else
			{
				if (state == 1 && c == '\\') // Module name
					module_pos = i;
				
				if (noDelimiter)
				{
					if (c != ' ' && c != ',' && c != '=' && c != '\0')
						continue;
				}
				else
				{
					if (c != '"')
						continue;
				}
				
				String module;
				String txt;
				
				if (state == 1 && module_pos >= 0)
				{
					module = line.substring(pos, module_pos);
					txt = line.substring(module_pos + 1, i);
				}
				else
				{
					module = null;
					txt = line.substring(pos, i);
				}
				
				// Found something
				switch (state)
				{
					case 1:
					{
						name = txt;
						protocol = module;
						break;
					}
					case 3:
					{
						if (isInvalidExension(new File(txt)))
							return;
						
						img = mep.createImage(txt, protocol);
						mep.setEmoticonImage(protocol, name, img);
						
						break;
					}
					case 5:
					{
						img.frame = max(0, Integer.parseInt(txt) - 1);
						break;
					}
				}
				
				state++;
			}
		}
	}
	
	private void addImages(File path, String protocol)
	{
		for (File file : path.listFiles())
		{
			if (isInvalidExension(file))
				continue;
			
			String imagePath = (protocol == null ? "" : protocol + "\\") + file.getName();
			ImageFile image = mep.createImage(imagePath, protocol);
			
			String name = removeExtension(file.getName());
			mep.setEmoticonImage(protocol, name, image);
		}
	}
	
	private String removeExtension(String name)
	{
		return name.substring(0, name.lastIndexOf('.'));
	}
	
	public static boolean isInvalidExension(File file)
	{
		return !file.getName().endsWith(".jpg") && !file.getName().endsWith(".jpeg") && !file.getName().endsWith(".png")
				&& !file.getName().endsWith(".gif") && !file.getName().endsWith(".swf");
	}
	
	public void save() throws IOException
	{
		mep.path.mkdirs();
		FileWriter out = new FileWriter(new File(mep.path, mep.path.getName() + ".mep"));
		
		if (!mep.name.isEmpty() && !mep.name.equals(mep.path.getName()))
			out.write("Name: " + mep.name + "\r\n");
		if (!mep.creator.isEmpty())
			out.write("Creator: " + mep.creator + "\r\n");
		if (!mep.udaterURL.isEmpty())
			out.write("Updater URL: " + mep.udaterURL + "\r\n");
		
		out.write("\r\n");
		
		writeEmoticonsForProtocol(out, null);
		for (String protocol : protocols)
			writeEmoticonsForProtocol(out, protocol);
		
		out.close();
	}
	
	private void writeEmoticonsForProtocol(FileWriter out, String protocol) throws IOException
	{
		StringBuilder sb = new StringBuilder();
		fillBuffer(sb, protocol);
		String txt = sb.toString();
		if (txt.isEmpty())
			return;
		out.write("# " + (protocol == null ? "Global" : protocol) + "\r\n");
		out.write(txt);
		out.write("\r\n");
	}
	
	private void fillBuffer(StringBuilder out, String protocol) throws IOException
	{
		for (Emoticon emo : mep.emoticons)
		{
			EmoticonImage image = emo.icons.get(protocol);
			if (image == null || image.image == null)
				continue;
			String protoPrefix = (protocol == null ? "" : protocol + "\\");
			String pathWithoutExtension = removeExtension(image.image.path);
			if (image.image.frame == 0 && pathWithoutExtension.equalsIgnoreCase(protoPrefix + emo.name))
				continue;
			
			out.append("\"" + protoPrefix + emo.name + "\" = \"" + image.image.path + "\"");
			if (image.image.frame != 0)
				out.append(", " + (image.image.frame + 1));
			out.append("\r\n");
		}
	}
}
