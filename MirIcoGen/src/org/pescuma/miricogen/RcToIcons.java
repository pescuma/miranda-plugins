package org.pescuma.miricogen;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.MessageBox;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.pescuma.jfg.gui.swt.JfgFormComposite;
import org.pescuma.jfg.gui.swt.JfgFormData;
import org.pescuma.jfg.reflect.ReflectionGroup;

public class RcToIcons
{
	private static final Pattern includePattern = Pattern.compile("#include\\s+\"([^\"]+)\"(\\s+.*)?");
	private static final Pattern definePattern = Pattern.compile("#define\\s+([^\"]+)\\s+([0-9]+)(\\s+.*)?");
	private static final Pattern icoPattern = Pattern.compile("([^ ]+)\\s+ICON\\s+(DISCARDABLE\\s+)?\"?([^\"]+)\"?(\\s+.*)?",
			Pattern.CASE_INSENSITIVE);
	
	private Config cfg = new Config();
	private Text out;
	
	public class Config
	{
		public String pluginName = "";
		public String iconsPrefix = "";
		public boolean useIdAsName = false;
		public File rcFile;
		public File outPath = new File("plugins");
	}
	
	public void show(Shell parent, File pluginsPath)
	{
		cfg.outPath = pluginsPath;
		
		Display display = parent.getDisplay();
		final Shell shell = new Shell(parent, SWT.DIALOG_TRIM | SWT.APPLICATION_MODAL);
		shell.setLayout(new GridLayout(1, true));
		
		final JfgFormComposite form = new JfgFormComposite(shell, SWT.NONE, new JfgFormData(JfgFormData.DIALOG));
		form.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		form.addContentsFrom(new ReflectionGroup(cfg));
		
		Button ok = new Button(shell, SWT.PUSH);
		ok.setLayoutData(new GridData(GridData.HORIZONTAL_ALIGN_CENTER));
		ok.addListener(SWT.Selection, new Listener() {
			public void handleEvent(Event event)
			{
				form.copyToModel();
				
				try
				{
					process();
				}
				catch (Exception e)
				{
					out.append("[ERROR] " + e.getMessage() + "\n\n\n");
					
					e.printStackTrace();
					MessageBox mb = new MessageBox(shell, SWT.OK | SWT.ERROR);
					mb.setText("Error");
					mb.setMessage(e.getMessage());
					mb.open();
				}
			}
		});
		ok.setText("Create .icons");
		
		out = new Text(shell, SWT.BORDER | SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL);
		out.setLayoutData(new GridData(GridData.FILL_BOTH));
		
		shell.setText(".rc to .icons Importer");
		shell.setDefaultButton(ok);
		shell.pack();
		shell.setSize(500, 300);
		shell.open();
		while (!shell.isDisposed())
		{
			if (!display.readAndDispatch())
				display.sleep();
		}
	}
	
	protected void process() throws IOException, InterruptedException
	{
		out.append("Starting process...\n\n");
		
		File file = cfg.rcFile;
		RC rc = loadRC(file);
		loadHs(rc);
		findDefs(rc);
		
		if (rc.icons.size() < 1)
		{
			out.append("[ERROR] No icons found\n");
			return;
		}
		
		outputFile(rc);
		
		out.append("\n");
		out.append("Done\n\n\n");
	}
	
	private void outputFile(RC rc)
	{
		File icons = new File(cfg.outPath, cfg.pluginName + ".icons");
		icons.getParentFile().mkdirs();
		
		FileWriter writer = null;
		try
		{
			writer = new FileWriter(icons, false);
			for (RCIcon icon : rc.icons)
			{
				writer.append(cfg.iconsPrefix);
				if (cfg.useIdAsName)
					writer.append(Integer.toString(icon.id));
				else
					writer.append(icon.name);
				writer.append("=");
				writer.append(Integer.toString(icon.id));
				writer.append("\r\n");
			}
		}
		catch (IOException e)
		{
			out.append("[ERROR] Error writting file: " + e.getMessage());
			e.printStackTrace();
		}
		finally
		{
			if (writer != null)
			{
				try
				{
					writer.close();
				}
				catch (IOException e)
				{
					e.printStackTrace();
				}
			}
		}
	}
	
	private void findDefs(RC rc)
	{
		for (Iterator<RCIcon> it = rc.icons.iterator(); it.hasNext();)
		{
			RCIcon ico = it.next();
			if (ico.def == null)
				continue;
			
			if (!rc.defs.containsKey(ico.def))
			{
				out.append("[ERROR] Define not found in .h: " + ico.def + "\n");
				it.remove();
			}
			else
				ico.id = rc.defs.get(ico.def);
		}
	}
	
	private RC loadRC(File file)
	{
		if (!file.exists())
			throw new IllegalStateException(".rc file don't exist: " + file.getAbsolutePath());
		
		BufferedReader in = null;
		try
		{
			RC rc = new RC();
			
			in = new BufferedReader(new FileReader(file));
			String line;
			while ((line = in.readLine()) != null)
			{
				int pos = line.indexOf("//");
				if (pos >= 0)
					line = line.substring(0, pos);
				
				line = line.replaceAll("\\s+", " ").trim();
				if (line.isEmpty())
					continue;
				
				Matcher m = includePattern.matcher(line);
				if (m.matches())
				{
					rc.hs.add(new File(file.getParentFile(), m.group(1)));
					continue;
				}
				
				m = icoPattern.matcher(line);
				if (m.matches())
				{
					RCIcon i = new RCIcon();
					
					String def = m.group(1).trim();
					File icoFile = new File(file.getParentFile(), m.group(3));
					
					if (isNumber(def))
					{
						i.def = null;
						i.id = Integer.parseInt(def);
					}
					else
					{
						i.def = def;
						
						if (def.startsWith("IDI_"))
							i.name = def.substring(4).toLowerCase();
					}
					
					if (i.name == null)
					{
						i.name = icoFile.getName();
						int index = i.name.indexOf('.');
						if (index >= 0)
							i.name = i.name.substring(0, index);
					}
					
					rc.icons.add(i);
					continue;
				}
			}
			
			return rc;
		}
		catch (IOException e)
		{
			throw new RuntimeException("Error loading .rc file", e);
		}
		finally
		{
			if (in != null)
			{
				try
				{
					in.close();
				}
				catch (IOException e)
				{
					e.printStackTrace();
				}
			}
		}
	}
	
	private boolean isNumber(String def)
	{
		for (int i = 0; i < def.length(); i++)
		{
			char c = def.charAt(i);
			if (c < '0' || c > '9')
				return false;
		}
		return true;
	}
	
	private void loadHs(RC rc)
	{
		for (int i = 0; i < rc.hs.size(); i++)
		{
			loadH(rc, rc.hs.get(i));
		}
	}
	
	private void loadH(RC rc, File file)
	{
		if (!file.exists())
		{
			out.append("[WARN] .h not fount: " + file.getAbsolutePath() + "\n");
			return;
		}
		
		BufferedReader in = null;
		try
		{
			in = new BufferedReader(new FileReader(file));
			String line;
			while ((line = in.readLine()) != null)
			{
				int pos = line.indexOf("//");
				if (pos >= 0)
					line = line.substring(0, pos);
				
				line = line.replaceAll("\\s+", " ").trim();
				if (line.isEmpty())
					continue;
				
				Matcher m = includePattern.matcher(line);
				if (m.matches())
				{
					rc.hs.add(new File(file.getParentFile(), m.group(1)));
					continue;
				}
				
				m = definePattern.matcher(line);
				if (m.matches())
				{
					rc.defs.put(m.group(1).trim(), Integer.parseInt(m.group(2)));
				}
			}
		}
		catch (IOException e)
		{
			out.append("[ERROR] Error loading .h file");
			e.printStackTrace();
		}
		finally
		{
			if (in != null)
			{
				try
				{
					in.close();
				}
				catch (IOException e)
				{
					e.printStackTrace();
				}
			}
		}
	}
	
	private static class RC
	{
		final List<File> hs = new ArrayList<File>();
		final List<RCIcon> icons = new ArrayList<RCIcon>();
		final Map<String, Integer> defs = new HashMap<String, Integer>();
	}
	
	private static class RCIcon
	{
		String name;
		String def;
		int id = -1;
	}
}
