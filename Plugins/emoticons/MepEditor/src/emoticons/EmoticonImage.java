package emoticons;

public class EmoticonImage
{
	final Emoticon emoticon;
	final String protocol;
	String description;
	ImageFile image;
	
	public EmoticonImage(Emoticon emo, String protocol)
	{
		this.emoticon = emo;
		this.protocol = protocol;
	}
	
	public EmoticonImage(Emoticon emo, String protocol, String description)
	{
		this.emoticon = emo;
		this.protocol = protocol;
		this.description = description;
	}
	
}
