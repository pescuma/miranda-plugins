package emoticons;
import java.util.HashMap;
import java.util.Map;

import org.eclipse.swt.widgets.Composite;

public class Emoticon
{
	String name;
	final Map<String, EmoticonImage> icons = new HashMap<String, EmoticonImage>();
	
	Composite frames;
	
	@Override
	public int hashCode()
	{
		final int prime = 31;
		int result = 1;
		result = prime * result + ((name == null) ? 0 : name.hashCode());
		return result;
	}
	
	@Override
	public boolean equals(Object obj)
	{
		if (this == obj)
			return true;
		if (obj == null)
			return false;
		if (getClass() != obj.getClass())
			return false;
		final Emoticon other = (Emoticon) obj;
		if (name == null)
		{
			if (other.name != null)
				return false;
		}
		else if (!name.equals(other.name))
			return false;
		return true;
	}
}
